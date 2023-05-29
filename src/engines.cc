/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// engines.cc
// search engines related code

#include "engines.hh"
#include <sstream>
#include <iostream>

#include <lexbor/core/base.h>
#include <lexbor/core/types.h>
#include <lexbor/html/parser.h>
#include <lexbor/html/serialize.h>
#include <lexbor/dom/collection.h>
#include <lexbor/dom/interfaces/element.h>

#include <chrono>
#include <thread>
#include <boost/algorithm/string/predicate.hpp>

namespace ranktracker {
  namespace engine {

    lxb_status_t
    serializer_callback(const lxb_char_t *data, size_t len, void *)
    {
      printf("%.*s", (int) len, (const char *) data);

      return LXB_STATUS_OK;
    }

    void
    serialize(lxb_dom_node_t *node)
    {
      lxb_status_t status;

      status = lxb_html_serialize_pretty_tree_cb(node,
                                                 LXB_HTML_SERIALIZE_OPT_UNDEF,
                                                 0, serializer_callback, NULL);
      if (status != LXB_STATUS_OK) {
        BOOST_LOG_TRIVIAL(error) << "Failed to serialization HTML tree\n";
      }
    }

    void
    serialize_node(lxb_dom_node_t *node)
    {
      lxb_status_t status;

      status = lxb_html_serialize_pretty_cb(node, LXB_HTML_SERIALIZE_OPT_UNDEF,
                                            0, serializer_callback, NULL);
      if (status != LXB_STATUS_OK) {
        BOOST_LOG_TRIVIAL(error) << "Failed to serialization HTML tree\n";
      }
    }


    class dom_collection {
      lxb_dom_collection_t *_col;
    public:
      dom_collection(lxb_dom_document_t *doc) {
        _col = lxb_dom_collection_make(doc, 10);
      }

      ~dom_collection() {
        lxb_dom_collection_destroy(_col, true);
      }

      lxb_dom_collection_t *operator() () {
        return _col;
      }
    };

    class html_document {
      lxb_html_document_t *_doc;
    public:
      html_document() {
        _doc = lxb_html_document_create();
      }

      html_document(html_document const&) = delete;
      html_document& operator= (const html_document &) = delete;

      void swap(html_document&& doc) {
        std::swap(_doc, doc._doc);
      }

      ~html_document() {
        lxb_html_document_destroy(_doc);
      }

      lxb_html_document_t *operator() () {
        return _doc;
      }
    };

    lxb_dom_element_t *get_child(lxb_dom_document_t *document,
                                 lxb_dom_element_t *root,
                                 const lxb_char_t *tag_name,
                                 size_t tag_name_len,
                                 int idx) {
      lxb_status_t dom_status;
      lxb_dom_element_t *el;
      dom_collection children(document);
      if(children() == NULL) {
        BOOST_LOG_TRIVIAL(error) << "Could not allocate dom collection on google_next_page function.\n";
        throw dom_exception();
      }
      dom_status = lxb_dom_elements_by_tag_name(root,
                                                children(),
                                                tag_name,
                                                tag_name_len);
      if(dom_status != LXB_STATUS_OK) {
        BOOST_LOG_TRIVIAL(error) << "ERROR: failed to get the first div in the footer\n";
        throw dom_exception();
      }
      el = lxb_dom_collection_element(children(), idx);
      return el;
    }

    std::string google_next_page(lxb_html_document_t *document) {
      assert(document != NULL);

      lxb_html_body_element_t *body = lxb_html_document_body_element(document);
      assert(body != NULL);

      // the footer
      lxb_dom_element_t *el = get_child(lxb_dom_interface_document(document),
                                        lxb_dom_interface_element(body),
                                        (const lxb_char_t *)"footer",
                                        6,
                                        0);
      if(!el) throw next_link_not_found();
      //serialize_node(lxb_dom_interface_node(el));

      // footer->div(0)->div(0)->div(0)
      el = get_child(lxb_dom_interface_document(document),
                     el,
                     (const lxb_char_t *)"div",
                     3,
                     2);
      if(!el) throw next_link_not_found();
      //serialize_node(lxb_dom_interface_node(el));

      // footer(0)->div(0)->div(0)->div(0)->a(1)
      lxb_dom_element_t *a = get_child(lxb_dom_interface_document(document),
                                       el,
                                       (const lxb_char_t *)"a",
                                       1,
                                       2);
      if(a == NULL /* maybe we are on the second page */) {
        // body->div(0)->footer(0)->div(0)->div(0)->div(0)->a(0)
        a = get_child(lxb_dom_interface_document(document),
                      el,
                      (const lxb_char_t *)"a",
                      1,
                      1);
      }

      if(a == NULL /* maybe we are on the first page */) {
        // body->div(0)->footer(0)->div(0)->div(0)->div(0)->a(0)
        a = get_child(lxb_dom_interface_document(document),
                      el,
                      (const lxb_char_t *)"a",
                      1,
                      0);
      }

      if(a == NULL) {
        BOOST_LOG_TRIVIAL(trace) << "no next link in google search response page\n";
        throw next_link_not_found();
      }

      //serialize_node(lxb_dom_interface_node(a));

      size_t href_len;
      const lxb_char_t *href;
      href = lxb_dom_element_get_attribute(a,
                                           (const lxb_char_t *)"href",
                                           4,
                                           &href_len);
      if(href_len && href) {
        return std::string((const char *)href, href_len);
      }
      throw next_link_not_found();
    }

    lxb_dom_element_t *find_element_by_id(lxb_html_document_t *document, const std::string& id) {
      dom_collection elements(lxb_dom_interface_document(document));
      lxb_status_t dom_status;
      dom_status = lxb_dom_elements_by_attr(lxb_dom_interface_element(document->body),
                                            elements(),
                                            (const lxb_char_t *)"id",
                                            2,
                                            (const lxb_char_t *)id.c_str(),
                                            id.size(),
                                            true);
      if(dom_status != LXB_STATUS_OK) {
        return NULL;
      }

      return lxb_dom_collection_element(elements(), 0);
    }

    size_t rcv_google_chunk(char *ptr, size_t, size_t nmemb, void *userdata) {
      lxb_html_document_t *doc = (lxb_html_document_t *)userdata;
      lxb_status_t result = lxb_html_document_parse_chunk(doc, (const lxb_char_t *)ptr, nmemb);

      if(result != LXB_STATUS_OK) {
        BOOST_LOG_TRIVIAL(error) << "Error: failed to parse html chunk from google\n";
        return 0;
      }

      return nmemb;
    }

    bool is_my_domain(const std::string &domain, const std::string &result_url) {
      return boost::algorithm::istarts_with(result_url, domain);
    }

    bool is_result_line(lxb_dom_node_t *node, std::string& result_url) {
      BOOST_LOG_TRIVIAL(trace) << "is_result_line() called\n";
      if(node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        BOOST_LOG_TRIVIAL(trace) << "is_reuslt_line(): node type is not an element\n";
        BOOST_LOG_TRIVIAL(trace) << "is_result_line() exit\n";
        return false;
      }
      if(node->first_child == NULL ||
         node->first_child->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        BOOST_LOG_TRIVIAL(trace) << "is_reuslt_line(): first child is not an element\n";
        BOOST_LOG_TRIVIAL(trace) << "is_result_line() exit\n";
        return false;
      }
      if(node->first_child->first_child == NULL ||
         node->first_child->first_child->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        BOOST_LOG_TRIVIAL(trace) << "is_reuslt_line(): first child of first child is not an element\n";
        BOOST_LOG_TRIVIAL(trace) << "is_result_line() exit\n";
        return false;
      }
      if(node->first_child->first_child->first_child == NULL ||
         node->first_child->first_child->first_child->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        BOOST_LOG_TRIVIAL(trace) << "is_reuslt_line(): first child of first child of first child "
          "is not an element\n";
        BOOST_LOG_TRIVIAL(trace) << "is_result_line() exit\n";
        return false;
      }

      size_t href_len;
      const lxb_char_t *href;
      href = lxb_dom_element_get_attribute(lxb_dom_interface_element(node->first_child->
                                                                     first_child->first_child),
                                           (const lxb_char_t *)"href",
                                           4,
                                           &href_len);
      if(href_len && href) {
        std::string candidate_url((const char *)href, href_len);
        BOOST_LOG_TRIVIAL(trace) << "is_result_line() found candidate url:"
                                 << candidate_url << std::endl;
        if(boost::algorithm::istarts_with(candidate_url, "/url?q=http")) {
          result_url = candidate_url.substr(7);
          BOOST_LOG_TRIVIAL(trace) << "is_result_line() found result url: "
                                   << result_url << std::endl;
          BOOST_LOG_TRIVIAL(trace) << "is_result_line() exit\n";
          return true;
        }
      }
      BOOST_LOG_TRIVIAL(trace) << "is_result_line() result url not found\n";
      BOOST_LOG_TRIVIAL(trace) << "is_result_line() exit\n";
      return false;
    }


    rank_result_type
    GoogleEngine::perform_rank_query(std::string domain,
                                     std::string keywords,
                                     progress_updater& p,
                                     std::string* page_url) const {
      BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): enter\n";
      auto curl_session = curl_easy_init();
      if(curl_session == NULL) {
        BOOST_LOG_TRIVIAL(error) << "GoogleEngine::perform_rank_query(): curl session could not be initialized\n";
        BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): exit\n";
        throw http_init_exception();
      }

      std::stringstream search_url;
      auto escaped_keywords = curl_easy_escape(curl_session, keywords.c_str(), keywords.length());
      search_url << url() << "/search?q=" << escaped_keywords;
      curl_free(escaped_keywords);
      BOOST_LOG_TRIVIAL(trace) << "get google page " << search_url.str() << std::endl;

      html_document document;
      lxb_status_t parser_status;

      if(document() == NULL) {
        BOOST_LOG_TRIVIAL(error) << "ERROR: failed to allocate new DOM document\n";
        BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): exit\n";
        throw parse_init_exception();
      }

      curl_easy_setopt(curl_session, CURLOPT_URL, search_url.str().c_str());
      curl_easy_setopt(curl_session, CURLOPT_WRITEFUNCTION, rcv_google_chunk);
      int crt_rank = 0;
      bool page_found = false;
      int page_rank = -1;
    next_page:
      BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): Loading and parsing next page\n";
      parser_status = lxb_html_document_parse_chunk_begin(document());
      if(parser_status != LXB_STATUS_OK) {
        BOOST_LOG_TRIVIAL(error) << "ERROR: failed to init parsing document chunks\n";
        BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): exit\n";
        throw parse_init_exception();
      }
      curl_easy_setopt(curl_session, CURLOPT_WRITEDATA, (void *)document());

      auto result = curl_easy_perform(curl_session);
      if(result != 0) {
        BOOST_LOG_TRIVIAL(error) << "Failed to fetch URL: " << search_url.str() << std::endl;
        BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): exit\n";
        throw http_request_failed_exception(result);
      }

      parser_status = lxb_html_document_parse_chunk_end(document());
      if(parser_status != LXB_STATUS_OK) {
        BOOST_LOG_TRIVIAL(error) << "ERROR: failed to end the google response parsing\n";
        BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): exit\n";
        throw parse_end_exception();
      }
      //ranktracker::engine::serialize(lxb_dom_interface_node(document()));

      // process the document to extract ranking info
      BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): finding main div\n";
      lxb_dom_element_t *maindiv = find_element_by_id(document(), "main");
      if(maindiv) {
        BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): main div found\n";
        for(auto node = lxb_dom_interface_node(maindiv)->first_child;
            node != NULL && !page_found && crt_rank < 100;
            node = node->next) {
          BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): "
            "getting next result node on crt page\n";

          std::string result_url;
          if(is_result_line(node, result_url)) {
            crt_rank++;
            BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_quer(): the current node "
              "is a result line; crt_rank = " << crt_rank << std::endl;

            if(is_my_domain(domain, result_url)) {
              BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_quer(): domain found; url: " << result_url << std::endl;
              page_found = true;
              page_rank = crt_rank;
              if (page_url) *page_url = result_url;
            }
          } else {
            BOOST_LOG_TRIVIAL(trace) << "The current node is not a result line\n";
          }
        }
      } else {
        BOOST_LOG_TRIVIAL(warning) << "GoogleEngine::perform_rank_query(): main div not found on Google results page\n";
      }
      if(crt_rank < 100 && !page_found) {
        BOOST_LOG_TRIVIAL(trace) << "did not reach the max rank and the domain was not found yet.\n";
        BOOST_LOG_TRIVIAL(trace) << "trying the next Google page\n";
        try {
          std::string next_page_url = url() + google_next_page(document());
          BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): get next page - "
                                   << next_page_url << std::endl;

          curl_easy_setopt(curl_session, CURLOPT_URL, next_page_url.c_str());
          document.swap(html_document()); // automatic free the prev document

          if(document() == NULL) {
            BOOST_LOG_TRIVIAL(error) << "ERROR: failed to allocate new DOM document\n";
            BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): exit\n";
            throw parse_init_exception();
          }
          BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): update progress with "
                                   << crt_rank << std::endl;
          p(crt_rank); // update progress
          BOOST_LOG_TRIVIAL(trace) << "waiting for 13s before fetching a new page\n";
          std::this_thread::sleep_for(std::chrono::seconds(13));
          goto next_page;
        } catch (next_link_not_found) {
          BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): no next page on google search\n";
        }
      }
      BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): finalizing search\n";
      BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): updating progress to max rank (100)\n";
      p(100); // update progress with maximum rank (job completed)
      if(page_found) {
        BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): page found  - rank is "
                                 << page_rank << std::endl;
      } else {
        BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query():page not found\n";
      }
      BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): cleaning up curl\n";
      curl_easy_cleanup(curl_session);

      BOOST_LOG_TRIVIAL(trace) << "GoogleEngine::perform_rank_query(): exit\n";
      return page_rank;
    }

    static engines_map engines;

    const boost::uuids::string_generator uuid_read;
    static GoogleEngine google_com(uuid_read("134d93aa-39c7-40c8-9834-c882b09ae93a"),
                                   "google.com",
                                   "Google",
                                   "https://www.google.com");
    static GoogleEngine google_uk(uuid_read("6d984c2d-0f13-4da0-ae7c-e7f6f5f47e74"),
                                  "google.uk",
                                  "Google/UK",
                                  "https://www.google.co.uk");

    void init_search_engines() {
      engines.insert({google_com.id(), SearchEngineRef(&google_com)});
      engines.insert({google_uk.id(), SearchEngineRef(&google_uk)});
    }

    const engines_map& search_engines() {
      return engines;
    }
  }
}
