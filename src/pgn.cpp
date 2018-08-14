#include "pgn.h"

#include <boost/optional.hpp>
#include <istream>

PGNParser::PGNParser(std::istream& is)
  : is_(is) {
}

boost::optional<int> parse_result(const std::string& result) {
  if (result == "1-0") {
    return 1;
  } else if (result == "0-1") {
    return -1;
  } else if (result == "1/2-1/2") {
    return 0;
  }
  return boost::none;
}

std::unique_ptr<PGNGame> PGNParser::parse() {
  // Skip all the PGN headers except Result and FEN
  std::string s;
  std::string result;
  std::string fen;

  const std::string kResultToken = "[Result \"";
  const std::string kFENToken = "[FEN \"";
  for (;;) {
    getline(is_, s);
    if (s.substr(0, kResultToken.size()) == kResultToken) {
        result = s.substr(kResultToken.size(), s.size() - kResultToken.size() - 2);
    } else if (s.substr(0, kFENToken.size()) == kFENToken) {
        fen = s.substr(kFENToken.size(), s.size() - kFENToken.size() - 2);
        fprintf(stderr, "Found fen %s\n", fen.c_str());
    }
    if (s.empty()) {
      break;
    }
  }

  if (is_.eof()) {
    return nullptr;
  }

  std::unique_ptr<PGNGame> game(new PGNGame);
  if (fen.empty()) {
    fprintf(stderr, "No fen\n");
    game->bh.set(Position::StartFEN);
  } else {
    game->bh.set(fen);
  }


  auto game_result = parse_result(result);
  if (game_result) {
    game->result = game_result.get();
  } else {
    throw std::runtime_error("Unknown result: " + result);
  }

  // Read in the moves
  for (int i = 0;;) {
    is_ >> s;
    if (!is_.good() || s.empty() || s == "[Event" || parse_result(s)) {
      break;
    }

    // Skip comments
    if (s.front() == '{' || s.front() == '[' || s.back() == '}' || s.back() == ']') {
      continue;
    }

    // Skip the move numbers
    if (s.back() == '.') {
      continue;
    }

    // Drop annotations
    if (s.back() == '!' || s.back() == '?') {
      size_t len = s.length();
      auto backIt = s.end();
      backIt--;
      while (*backIt == '!' || *backIt == '?') {
        backIt--;
        len--;
      }
      s = s.substr(0, len);
    }

    ++i;
    Move m = game->bh.cur().san_to_move(s);
    if (m == MOVE_NONE) {
      throw std::runtime_error("Unable to parse pgn move " + s);
    }
    game->bh.do_move(m);
  }

  // Read the empty line after the game
  getline(is_, s);
  getline(is_, s);

  return game;
}
