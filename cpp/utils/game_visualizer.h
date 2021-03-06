#ifndef CPP_UTILS_GAME_VISUALIZER_H_
#define CPP_UTILS_GAME_VISUALIZER_H_

#include <string>
#include <iostream>
#include <map>
#include "game/position.h"
#include "game/race.h"

#include "game/result.h"

#include "gflags/gflags.h"

DECLARE_bool(disable_visualizer);

namespace utils {

class GameVisualizer {
 public:
  GameVisualizer() : lap_times_() {}

  void Print() {
    if (FLAGS_disable_visualizer) return;
    if (print_) {
      if (!visible_)
        visible_ = true;

      for (auto car : positions_)
        PrintCar(car.first, car.second);

      CarriageReturn(positions_.size() * kPlayerLines + 1);
    }
  }

  void Update(std::map<std::string, game::Position> positions) {
    if (FLAGS_disable_visualizer) return;
    positions_ = positions;
    Print();
  }

  void LapFinished(const game::Result& result) {
    if (FLAGS_disable_visualizer) return;
    if (lap_times_.find(result.color()) == lap_times_.end())
      lap_times_[result.color()] = vector<int>();

    lap_times_[result.color()].push_back(result.lap_time());
    Print();
  }

  void CarCrashed(const std::string& color) {
    if (FLAGS_disable_visualizer) return;
    if (crashes_.find(color) == crashes_.end())
      crashes_[color] = vector<std::pair<int, int>>();

    crashes_[color].push_back(std::make_pair(positions_[color].lap(), positions_[color].piece()));
    Print();
  }

  void GameEnd() {
    if (FLAGS_disable_visualizer) return;
    lap_times_.clear();
    print_ = false;
    std::cout << std::endl;
  }

  void GameStart() { print_ = true; }

  void CarFinishedRace(const std::string& color) { Print(); }

  void set_race(const game::Race& race) {
    race_ = race;
    pieces_ = race_.track().pieces().size();
  }

 private:
  void PrintCar(const std::string& color, const game::Position& position) {
    std::cout << "                                                                                                " << std::endl;
    PrintLapTimes(color);
    std::cout << "                                                                                                " << std::endl;
    PrintTrackInfo(color, position);
    std::cout << "                                                                                                " << std::endl;
  }

  void PrintTrackInfo(const std::string& color, const game::Position& position) {
    PrintPlayerName(color);
    std::cout << "|";

    for (int lap = 0; lap < position.lap(); lap++)
      PrintFullLap(color, lap, pieces_);

    PrintPartialLap(color, position.lap(), position.piece() + 1);
  }

  void PrintPartialLap(const std::string& color, int lap, int pieces) {
    for (int i = 0; i < pieces; i++) {
      if (IsCrash(color, lap, i))
        std::cout << "\x1B[31mX\x1B[0m";
      else
        std::cout << "_";
    }
  }

  // 9 characters inside
  void PrintFullLap(const std::string& color, int lap, int pieces) {
    std::cout << "________|";
  }

  void PrintLapTimes(const std::string& color) {
    std::cout << std::setw(kHeaderLength) << " ";
    std::cout << "|";
    if (lap_times_.find(color) == lap_times_.end())
      return;

    for (int i = 0; i < lap_times_[color].size(); i++)
      std::cout << std::right << std::setw(6) << double(lap_times_[color][i]) / 1000.0 << "s |";
  }

  bool IsCrash(const std::string& color, int lap, int piece) {
    if (crashes_.find(color) == crashes_.end())
      return false;

    for (auto c : crashes_[color])
      if (c.first == lap && c.second == piece)
        return true;

    return false;
  }

  void CarriageReturn(int line_no) {
    while (--line_no)
      std::cout << "\x1b[A";
    std::cout << "\r";
    std::cout.flush();
  }

  void PrintPlayerName(const string& color) {
    std::map<std::string, std::string> colors {
      { "red", "\x1B[31m" },
      { "blue", "\x1B[34m" },
      { "green", "\x1B[32m" },
      { "yellow", "\x1B[33m" },
      { "normal", "\x1B[0m" }
    };

    for (auto c : race_.cars())
      if (c.color() == color) {
        std::cout << colors[color];
        std::string name = c.name();
        if (name[0] >= 'a' && name[0] <= 'z')
          name[0] += 'A'-'a';

        int left = (kHeaderLength + name.size()) / 2;
        int right = kHeaderLength - left;

        std::cout << std::right << std::setw(left) << name;
        std::cout << std::left << std::setw(right) << " ";

        std::cout << colors["normal"];
      }
  }

  const int kPlayerLines = 3;
  const int kHeaderLength = 16;
  bool visible_ = false;
  bool print_ = false;

  game::Race race_;
  int pieces_;
  std::map<std::string, std::vector<int>> lap_times_;
  std::map<std::string, std::vector<std::pair<int, int>>> crashes_;
  std::map<std::string, game::Position> positions_;
};

}  // namespace utils

#endif  // CPP_UTILS_GAME_VISUALIZER_H_
