
/**
 * @file schema.hpp
 * @brief defines Schema of each relation.
 *
 */
#include <array>
#include "../shared/Types.hpp"

struct scaled_t {
   // Entries: 1 to 1 160 000 * scale
   static constexpr int id = 0;
   struct Key {
      static constexpr int id = 0;
      Integer scaled_key;
   };
   Integer s_u_key;      // references unscaled: scaled_key % |unscaled| + 1
   Varchar<11> s_color;  // random color
   Numeric s_num;        // random Numeric
   // -------------------------------------------------------------------------------------
   template <class T>
   static unsigned foldKey(uint8_t* out, const T& key)
   {
      unsigned pos = 0;
      pos += fold(out + pos, key.scaled_key);
      return pos;
   }
   template <class T>
   static unsigned unfoldKey(const uint8_t* in, T& key)
   {
      unsigned pos = 0;
      pos += unfold(in + pos, key.scaled_key);
      return pos;
   }
   static constexpr unsigned maxFoldLength() { return 0 + sizeof(Key::scaled_key); };
};

struct bf_record_t {
   struct Key {
      Varchar<16> key;
   };

   Varchar<16> value;

   static Key make_key(uint64_t id)
   {
      Key k;

      std::memcpy(k.key.data, &id, sizeof(id));
      std::memcpy(k.key.data + sizeof(id), &id, sizeof(id));
      k.key.length = 16;
      return k;
   }

   static bf_record_t make_record(uint64_t id)
   {
      bf_record_t r;
      r.value = make_key(id).key;
      return r;
   }

   bool check_value(uint64_t id) const
   {
      uint64_t value_id;
      std::memcpy(&value_id, value.data, sizeof(value_id));
      return value_id == id;
   }

   template <class T>
   static unsigned foldKey(uint8_t* out, const T& key)
   {
      unsigned pos = 0;
      pos += fold(out + pos, key.key);
      return pos;
   }

   template <class T>
   static unsigned unfoldKey(const uint8_t* in, T& key)
   {
      unsigned pos = 0;
      pos += unfold(in + pos, key.key);
      return pos;
   }

   static constexpr unsigned maxFoldLength() { return 16; };
};
