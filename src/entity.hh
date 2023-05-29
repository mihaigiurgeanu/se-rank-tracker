// entity.hh
// defines Entity base class for objects that have persiostent
// identities.
#ifndef RANKTRACKER_ENTITY_HH
#define RANKTRACKER_ENTITY_HH

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/uuid/uuid_serialize.hpp>
#include "logging.hh"

namespace ranktracker {
  namespace data {

    class SerializationException {
    public:
      enum SerializationExceptionType {
        UnknownException,
        UnknownVersion
      };

    private:
      SerializationExceptionType _exception_type;

    protected:
      SerializationException() = delete;
      SerializationException(SerializationExceptionType t) :
        _exception_type(t)
      {}
    public:
      bool is(SerializationExceptionType t) {
        return t == _exception_type;
      }
    };

    class UnknownSerializationVersion : SerializationException {
      unsigned int _version;

    public:
      UnknownSerializationVersion(unsigned int version) :
        SerializationException(UnknownVersion),
        _version(version)
      {}

      unsigned int version() { return _version; }
    };


    struct AbstractEntity {
      typedef boost::uuids::uuid id_type;
      virtual const id_type& id() const = 0;
    };

    class Entity : public AbstractEntity {
      friend class boost::serialization::access;

      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
        if(version > 0) {
          throw UnknownSerializationVersion(version);
        }
        ar & _id;
      }

      id_type _id;
    public:
      Entity() : _id(boost::uuids::random_generator()())
      {}

      Entity(const id_type& id) : _id(id)
      {}

      const id_type& id() const
      { return _id; }
    };

    inline bool operator== (const AbstractEntity& a, const AbstractEntity& b) {
      return a.id() == b.id();
    }
  }
}

namespace std {

  template<>
  struct hash<ranktracker::data::AbstractEntity> {
    size_t operator() (const ranktracker::data::AbstractEntity& e) const {
      boost::hash<boost::uuids::uuid> uuid_hasher;
      return uuid_hasher(e.id());
    }
  };
}

namespace ranktracker {
  namespace data {
    typedef std::hash<ranktracker::data::AbstractEntity> Entity_hasher;
  }
}

#endif
