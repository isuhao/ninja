// Copyright 2011 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NINJA_PARSERS_H_
#define NINJA_PARSERS_H_

#include <string>
#include <vector>
#include <limits>

using namespace std;

struct BindingEnv;

/// A single parsed token in an input stream.
struct Token {
  enum Type {
    NONE,
    UNKNOWN,
    IDENT,
    NEWLINE,
    EQUALS,
    COLON,
    PIPE,
    PIPE2,
    INDENT,
    OUTDENT,
    TEOF
  };
  explicit Token(Type type) : type_(type) {}

  void Clear() { type_ = NONE; }
  string AsString() const;

  Type type_;
  const char* pos_;
  const char* end_;
};

/// Represents a user-understandable position within a source file.
struct SourceLocation {
  SourceLocation(int line, int col) : line_(line), column_(col) {}

  /// Construct an error message based on the position and message,
  /// write it into \a err, then return false.
  bool Error(const string& message, string* err);

  /// 1-based line and column numbers.
  int line_;
  int column_;
};

/// Processes an input stream into Tokens.
struct Tokenizer {
  Tokenizer()
    : makefile_flavor_(false),
      token_(Token::NONE), line_number_(0),
      last_indent_(0), cur_indent_(-1) {}

  /// Tokenization differs slightly between ninja files and Makefiles.
  /// By default we tokenize as ninja files; calling this changes to
  /// Makefile-style tokenization.
  void SetMakefileFlavor() {
    makefile_flavor_ = true;
  }

  void Start(const char* start, const char* end);
  /// Report an error with a location pointing at the current token.
  bool Error(const string& message, string* err);
  /// Call Error() with "expected foo, got bar".
  bool ErrorExpected(const string& expected, string* err);

  const Token& token() const { return token_; }

  void SkipWhitespace(bool newline=false);
  bool Newline(string* err);
  bool ExpectToken(Token::Type expected, string* err);
  bool ExpectIdent(const char* expected, string* err);
  bool ReadIdent(string* out);
  bool ReadToNewline(string* text, string* err,
                     size_t max_length=std::numeric_limits<size_t>::max());

  Token::Type PeekToken();
  void ConsumeToken();

  SourceLocation Location() {
    return SourceLocation(line_number_ + 1, token_.pos_ - cur_line_ + 1);
  }

  bool makefile_flavor_;

  const char* cur_;
  const char* end_;

  const char* cur_line_;
  Token token_;
  int line_number_;
  int last_indent_, cur_indent_;
};

/// Parses simple Makefiles as generated by gcc.
struct MakefileParser {
  MakefileParser();
  bool Parse(const string& input, string* err);

  Tokenizer tokenizer_;
  string out_;
  vector<string> ins_;
};

struct EvalString;
struct State;

/// Parses .ninja files.
struct ManifestParser {
  struct FileReader {
    virtual ~FileReader() {}
    virtual bool ReadFile(const string& path, string* content, string* err) = 0;
  };

  ManifestParser(State* state, FileReader* file_reader);

  bool Load(const string& filename, string* err);
  bool Parse(const string& input, string* err);

  bool ParseRule(string* err);
  /// Parse a key=val statement, expanding $vars in the value with the
  /// current env.
  bool ParseLet(string* key, string* val, string* err);
  bool ParseEdge(string* err);

  /// Parse either a 'subninja' or 'include' line.
  bool ParseFileInclude(string* err);


  /// Parse the "key=" half of a key=val statement.
  bool ParseLetKey(string* key, string* err);
  /// Parse the val half of a key=val statement, writing and parsing
  /// output into an EvalString (ready for expansion).
  bool ParseLetValue(EvalString* eval, string* err);

  State* state_;
  BindingEnv* env_;
  FileReader* file_reader_;
  Tokenizer tokenizer_;
};

#endif  // NINJA_PARSERS_H_
