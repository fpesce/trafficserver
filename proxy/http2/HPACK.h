/** @file

  A brief file description

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

#ifndef __HPACK_H__
#define __HPACK_H__

#include "libts.h"
#include "HTTP.h"

// Constant strings for pseudo headers of HPACK
extern const char *HPACK_VALUE_SCHEME;
extern const char *HPACK_VALUE_METHOD;
extern const char *HPACK_VALUE_AUTHORITY;
extern const char *HPACK_VALUE_PATH;
extern const char *HPACK_VALUE_STATUS;

extern const unsigned HPACK_LEN_SCHEME;
extern const unsigned HPACK_LEN_METHOD;
extern const unsigned HPACK_LEN_AUTHORITY;
extern const unsigned HPACK_LEN_PATH;
extern const unsigned HPACK_LEN_STATUS;

// It means that any header field can be compressed/decompressed by ATS
const static int HPACK_ERROR_COMPRESSION_ERROR = -1;
// It means that any header field is invalid in HTTP/2 spec
const static int HPACK_ERROR_HTTP2_PROTOCOL_ERROR = -2;

enum HpackFieldType {
  HPACK_FIELD_INDEX,              // HPACK 7.1 Indexed Header Field Representation
  HPACK_FIELD_INDEXED_LITERAL,    // HPACK 7.2.1 Literal Header Field with Incremental Indexing
  HPACK_FIELD_NOINDEX_LITERAL,    // HPACK 7.2.2 Literal Header Field without Indexing
  HPACK_FIELD_NEVERINDEX_LITERAL, // HPACK 7.2.3 Literal Header Field never Indexed
  HPACK_FIELD_TABLESIZE_UPDATE,   // HPACK 7.3 Header Table Size Update
};

class MIMEFieldWrapper
{
public:
  MIMEFieldWrapper(MIMEField *f, HdrHeap *hh, MIMEHdrImpl *impl) : _field(f), _heap(hh), _mh(impl) {}

  void
  name_set(const char *name, int name_len)
  {
    _field->name_set(_heap, _mh, name, name_len);
  }

  void
  value_set(const char *value, int value_len)
  {
    _field->value_set(_heap, _mh, value, value_len);
  }

  const char *
  name_get(int *length) const
  {
    return _field->name_get(length);
  }

  const char *
  value_get(int *length) const
  {
    return _field->value_get(length);
  }

  const MIMEField *
  field_get() const
  {
    return _field;
  }

private:
  MIMEField *_field;
  HdrHeap *_heap;
  MIMEHdrImpl *_mh;
};

// 2.3.2. Dynamic Table
class Http2DynamicTable
{
public:
  Http2DynamicTable() : _current_size(0), _settings_dynamic_table_size(4096)
  {
    _mhdr = new MIMEHdr();
    _mhdr->create();
  }

  ~Http2DynamicTable()
  {
    _headers.clear();
    _mhdr->fields_clear();
    delete _mhdr;
  }

  void add_header_field(const MIMEField *field);
  int get_header_from_indexing_tables(uint32_t index, MIMEFieldWrapper &header_field) const;
  bool set_dynamic_table_size(uint32_t new_size);

private:
  const MIMEField *
  get_header(uint32_t index) const
  {
    return _headers.get(index - 1);
  }

  const uint32_t
  get_current_entry_num() const
  {
    return _headers.length();
  }

  uint32_t _current_size;
  uint32_t _settings_dynamic_table_size;

  MIMEHdr *_mhdr;
  Vec<MIMEField *> _headers;
};

HpackFieldType hpack_parse_field_type(uint8_t ftype);

static inline bool
hpack_field_is_literal(HpackFieldType ftype)
{
  return ftype == HPACK_FIELD_INDEXED_LITERAL || ftype == HPACK_FIELD_NOINDEX_LITERAL || ftype == HPACK_FIELD_NEVERINDEX_LITERAL;
}

int64_t encode_integer(uint8_t *buf_start, const uint8_t *buf_end, uint32_t value, uint8_t n);
int64_t decode_integer(uint32_t &dst, const uint8_t *buf_start, const uint8_t *buf_end, uint8_t n);
int64_t encode_string(uint8_t *buf_start, const uint8_t *buf_end, const char *value, size_t value_len);
int64_t decode_string(Arena &arena, char **str, uint32_t &str_length, const uint8_t *buf_start, const uint8_t *buf_end);

int64_t encode_indexed_header_field(uint8_t *buf_start, const uint8_t *buf_end, uint32_t index);
int64_t encode_literal_header_field(uint8_t *buf_start, const uint8_t *buf_end, const MIMEFieldWrapper &header, uint32_t index,
                                    HpackFieldType type);
int64_t encode_literal_header_field(uint8_t *buf_start, const uint8_t *buf_end, const MIMEFieldWrapper &header,
                                    HpackFieldType type);

// When these functions returns minus value, any error occurs
// TODO Separate error code and length of processed buffer
int64_t decode_indexed_header_field(MIMEFieldWrapper &header, const uint8_t *buf_start, const uint8_t *buf_end,
                                    Http2DynamicTable &dynamic_table);
int64_t decode_literal_header_field(MIMEFieldWrapper &header, const uint8_t *buf_start, const uint8_t *buf_end,
                                    Http2DynamicTable &dynamic_table);
int64_t update_dynamic_table_size(const uint8_t *buf_start, const uint8_t *buf_end, Http2DynamicTable &dynamic_table);

#endif /* __HPACK_H__ */
