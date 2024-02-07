// Project Identifier: A8A3A33EF075ACEF9B08F5B9845569ECCB423725
#include <array>
#include <cstddef>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

#include <getopt.h>

using namespace std;

struct point {
  uint8_t color;
  uint32_t x_coordinate;
  uint32_t y_coordinate;
};

struct state {
  char coordinate_backtrace;
  char character;
  bool discovered;
};

// x is row meaning the y-axis and y is column meaning the x-axis
class puzzle {
public:
  vector<vector<vector<state> > > maze;
  uint32_t starting_x;
  uint32_t starting_y;
  uint32_t total_rows;
  uint32_t total_columns;
  uint32_t num_colors;
  char outMode;
  bool stack;
  bool queue;
  bool outputMap;
  bool outputList;
  bool solved;
};

option redirect[] = {
  {  "help",       no_argument, nullptr,  'h'},
  { "stack",       no_argument, nullptr,  's'},
  { "queue",       no_argument, nullptr,  'q'},
  {"output", required_argument, nullptr,  'o'},
  { nullptr,                 0, nullptr, '\0'}
};

int main(int argc, char* argv[]) {
  std::ios_base::sync_with_stdio(false);
  
  puzzle game;
  
  bool& stack_ref = game.stack;
  bool& queue_ref = game.queue;
  game.solved = false;
  stack_ref = false;
  queue_ref = false;
  int index = 0;
  int opt = 0;
  string outputMode = "No";

   while ((opt = getopt_long(argc, argv, "hsqo:", redirect, &index)) != -1) {
    switch (opt) {
      case 'h':
        cout << "This program finds a path out of a puzzle if there is one\n";
        cout << "These are the valid command line arguments:\n";
        cout << "'-s' or '--stack' will use a stack based path finding scheme\n";
        cout << "'-q' or '--queue' will use a queue based path finding scheme\n";
        cout << "'-o' or '--output' will indicate if you want a map or list output\n";
        cout << "'-h' or '--help' will print this message again and end the program\n";
        return 0;
        break;
      case 's':
        if (!queue_ref && !stack_ref) {
          stack_ref = true;
        } else {
          cerr << "Error: Must have only one stack or one queue\n";
          return 1;
        }
        break;
      case 'q':
        if (!queue_ref && !stack_ref) {
          queue_ref = true;
        } else {
          cerr << "Error: Must have only one stack or one queue\n";
          return 1;
        }
        break;
      case 'o': {
        game.outMode = *optarg;
        outputMode = string(optarg);

        if (outputMode == "map") {
          game.outputMap = true;
          game.outputList = false;
        } else if (outputMode == "list") {
          game.outputList = true;
          game.outputMap = false;
        } else {
          cerr << "Error: Invalid output mode\n";
          return 1;
        }
        break;
      }
      default:
        cerr << "Error: Unknown option\n";
        return 1;
    }
  }

  if(outputMode == "No") {
    game.outputMap = true;
    game.outputList = false;
  }

  if (!queue_ref && !stack_ref) {
    cerr << "Error: Must have at least one of stack or queue\n";
    return 1;
  }
  if (queue_ref && stack_ref) {
    cerr << "Error: Can not have both stack and queue\n";
    return 1;
  }

  string junk;
  cin >> game.num_colors >> game.total_rows >> game.total_columns;

  if (game.num_colors > 26) {
    cerr << "Error: Invalid numColor\n";
    return 1;
  }
  if (game.total_rows < 1) {
    cerr << "Error: Invalid height\n";
    return 1;
  }
  if (game.total_columns < 1) {
    cerr << "Error: Invalid width\n";
    return 1;
  }

  getline(cin, junk);
  vector<vector<char> > map(game.total_rows, vector<char>(game.total_columns));
  uint32_t x = 0;
  char c = ' ';
  uint8_t count_at = 0;
  uint8_t count_question = 0;

  while (cin >> c) {
    if ((c == '/') || (c == ' ')) {
      string trash;
      getline(cin, trash);
    } else {
      if(((static_cast<int>(c) > 96) && (static_cast<uint32_t>(c) < 97 + game.num_colors)) || ((static_cast<int>(c) > 64) 
            && (static_cast<uint32_t>(c) < 65 + game.num_colors)) || (c == '#') || (c == '.') || (c == '@') || (c == '?') 
            || (c == '^')) {
        map[x][0] = c;
        if (c == '@') {
          game.starting_x = x;
          game.starting_y = 0;
          ++count_at;
        }
        if (c == '?') {
          ++count_question;
        }
      } else {
        cerr << "Error: Invalid character in map input\n";
        return 1;
      } 
      for (uint32_t i = 1; i < game.total_columns; ++i) {
        cin >> c;
        if(((static_cast<int>(c) > 96) && (static_cast<uint32_t>(c) < 97 + game.num_colors)) || ((static_cast<int>(c) > 64) 
            && (static_cast<uint32_t>(c) < 65 + game.num_colors)) || (c == '#') || (c == '.') || (c == '@') || (c == '?') 
            || (c == '^')) {
          if (c == '@') {
            game.starting_x = x;
            game.starting_y = i;
            ++count_at;
          }
          if (c == '?') {
            ++count_question;
          }
          map[x][i] = c;
        } else {
          cerr << "Error: Invalid character in map input\n";
          return 1;
        }
      }
      ++x;
    } 
    /*else {
      cerr << "Error: Invalid character in map input\n";
      return 1;
    }*/
  }



  if ((count_at != 1) || (count_question != 1)) {
    cerr << "Error: Puzzle must have only one start and one target\n";
    return 1;
  }

  game.maze.resize(static_cast<uint32_t>(game.num_colors + 1),
                     vector<vector<state> >(game.total_rows, vector<state>(game.total_columns)));

  for (uint32_t k = 0; k < static_cast<uint32_t>(game.num_colors + 1); ++k) {
    for (uint32_t i = 0; i < game.total_rows; ++i) {
      for (uint32_t j = 0; j < game.total_columns; ++j) {
        state newPoint = {' ', map[i][j], false,};
          game.maze[k][i][j] = newPoint;
      }
    }
  }

  deque<point> search_container;
  point new_state = { 0, game.starting_x, game.starting_y };
  point current_state;
  search_container.push_back(new_state);
  game.maze[0][new_state.x_coordinate][new_state.y_coordinate].discovered = true;

  while (!game.solved && !search_container.empty()) {
    if(stack_ref) {
      current_state = search_container.back();
      search_container.pop_back();
    } else {
      current_state = search_container.front();
      search_container.pop_front();
    }

    // button check
    if ((game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate].character != static_cast<char>(current_state.color + 96))
          && ((game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate].character > 96)
          && (game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate].character < 123))) {
            if(!game.maze[static_cast<uint32_t>(game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate].character - 96)][current_state.x_coordinate][current_state.y_coordinate].discovered) {
                new_state.color = static_cast<uint8_t>(game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate].character - 96);

                new_state.x_coordinate = current_state.x_coordinate;
                new_state.y_coordinate = current_state.y_coordinate;

                search_container.push_back(new_state);
                game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = static_cast<char>(96 + current_state.color);
                game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
              }
            } else if ((game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate].character == '^') && (current_state.color != 0)) {
                  if((!game.maze[static_cast<uint32_t>(0)][current_state.x_coordinate][current_state.y_coordinate].discovered)) {
                    new_state.color = static_cast<uint32_t>(0);
                new_state.x_coordinate = current_state.x_coordinate;
                new_state.y_coordinate = current_state.y_coordinate;

                search_container.push_back(new_state);
                game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
                game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = static_cast<char>(96 + current_state.color);
                }
            } else {
                // check north
                if ((current_state.x_coordinate > 0)
                    && (!game.maze[current_state.color][current_state.x_coordinate - 1][current_state.y_coordinate]
                           .discovered)) {
                    if (game.maze[current_state.color][current_state.x_coordinate - 1][current_state.y_coordinate]
                          .character
                        == static_cast<char>(current_state.color + 64)) {
                        new_state.color = current_state.color;
                        new_state.x_coordinate = current_state.x_coordinate - 1;
                        new_state.y_coordinate = current_state.y_coordinate;
                        search_container.push_back(new_state);
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = 'S';

                    }
                    if ((game
                                  .maze[current_state.color][current_state.x_coordinate - 1][current_state.y_coordinate]
                                  .character
                                > 96)
                               && (game
                                     .maze[current_state.color][current_state.x_coordinate - 1]
                                          [current_state.y_coordinate]
                                     .character
                                   < 123)) {
                        new_state.color = current_state.color;
                        new_state.x_coordinate = current_state.x_coordinate - 1;
                        new_state.y_coordinate = current_state.y_coordinate;
                        search_container.push_back(new_state);
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = 'S';
                    }
                    if (game.maze[current_state.color][current_state.x_coordinate - 1]
                                        [current_state.y_coordinate]
                                          .character
                                 == '.'
                               || (game
                                     .maze[current_state.color][current_state.x_coordinate - 1]
                                          [current_state.y_coordinate]
                                     .character
                                   == '?')
                               || (game
                                     .maze[current_state.color][current_state.x_coordinate - 1]
                                          [current_state.y_coordinate]
                                     .character
                                   == '^')
                                || (game
                                     .maze[current_state.color][current_state.x_coordinate - 1]
                                          [current_state.y_coordinate]
                                     .character
                                   == '@')) {
                        new_state.color = current_state.color;
                        new_state.x_coordinate = current_state.x_coordinate - 1;
                        new_state.y_coordinate = current_state.y_coordinate;
                        if(game.maze[current_state.color][current_state.x_coordinate - 1][current_state.y_coordinate].character == '?') {
                          current_state = new_state;
                          game.solved = true;
                        } else {
                          search_container.push_back(new_state);

                        }
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = 'S';
                    }
                }

                // check east
                if ((current_state.y_coordinate < game.total_columns - 1)
                    && (!game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate + 1]
                           .discovered)) {
                    if (game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate + 1]
                          .character
                        == static_cast<char>(current_state.color + 64)) {
                        new_state.color = current_state.color;
                        new_state.x_coordinate = current_state.x_coordinate;
                        new_state.y_coordinate = current_state.y_coordinate + 1;
                        search_container.push_back(new_state);
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = 'W';
                    }
                    if ((game
                                  .maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate + 1]
                                  .character
                                > 96)
                               && (game
                                     .maze[current_state.color][current_state.x_coordinate]
                                          [current_state.y_coordinate + 1]
                                     .character
                                   < 123)) {
                        new_state.color = current_state.color;
                        new_state.x_coordinate = current_state.x_coordinate;
                        new_state.y_coordinate = current_state.y_coordinate + 1;
                        search_container.push_back(new_state);
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = 'W';
                    }
                    if (game.maze[current_state.color][current_state.x_coordinate]
                                        [current_state.y_coordinate + 1]
                                          .character
                                 == '.'
                               || (game
                                     .maze[current_state.color][current_state.x_coordinate]
                                          [current_state.y_coordinate + 1]
                                     .character
                                   == '?')
                               || (game
                                     .maze[current_state.color][current_state.x_coordinate]
                                          [current_state.y_coordinate + 1]
                                     .character
                                   == '^')
                                || (game
                                     .maze[current_state.color][current_state.x_coordinate]
                                          [current_state.y_coordinate + 1]
                                     .character
                                   == '@')) {
                        new_state.color = current_state.color;
                        new_state.x_coordinate = current_state.x_coordinate;
                        new_state.y_coordinate = current_state.y_coordinate + 1;
                        if(game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate + 1].character == '?') {
                          current_state = new_state;
                          game.solved = true;
                        } else {
                          search_container.push_back(new_state);

                        }
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = 'W';
                    }
                }

                // check south
                if ((current_state.x_coordinate < game.total_rows - 1)
                    && (!game.maze[current_state.color][current_state.x_coordinate + 1][current_state.y_coordinate]
                           .discovered)) {
                    if (game.maze[current_state.color][current_state.x_coordinate + 1][current_state.y_coordinate]
                          .character
                        == static_cast<char>(current_state.color + 64)) {
                        new_state.color = current_state.color;
                        new_state.x_coordinate = current_state.x_coordinate + 1;
                        new_state.y_coordinate = current_state.y_coordinate;
                        search_container.push_back(new_state);
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = 'N';
                    }
                    if ((game
                                  .maze[current_state.color][current_state.x_coordinate + 1][current_state.y_coordinate]
                                  .character
                                > 96)
                               && (game
                                     .maze[current_state.color][current_state.x_coordinate + 1]
                                          [current_state.y_coordinate]
                                     .character
                                   < 123)) {
                        new_state.color = current_state.color;
                        new_state.x_coordinate = current_state.x_coordinate + 1;
                        new_state.y_coordinate = current_state.y_coordinate;
                        search_container.push_back(new_state);
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = 'N';
                    }
                    if (game.maze[current_state.color][current_state.x_coordinate + 1]
                                        [current_state.y_coordinate]
                                          .character
                                 == '.'
                               || (game
                                     .maze[current_state.color][current_state.x_coordinate + 1]
                                          [current_state.y_coordinate]
                                     .character
                                   == '?')
                               || (game
                                     .maze[current_state.color][current_state.x_coordinate + 1]
                                          [current_state.y_coordinate]
                                     .character
                                   == '^')
                                || (game
                                     .maze[current_state.color][current_state.x_coordinate + 1]
                                          [current_state.y_coordinate]
                                     .character
                                   == '@')) {
                        new_state.color = current_state.color;
                        new_state.x_coordinate = current_state.x_coordinate + 1;
                        new_state.y_coordinate = current_state.y_coordinate;
                        if(game.maze[current_state.color][current_state.x_coordinate + 1][current_state.y_coordinate].character == '?') {
                          current_state = new_state;
                          game.solved = true;
                        } else {
                          search_container.push_back(new_state);

                        }
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = 'N';
                    }
                }

                // check west
                if ((current_state.y_coordinate > 0)
                    && (!game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate - 1]
                           .discovered)) {
                    if (game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate - 1]
                          .character
                        == static_cast<char>(current_state.color + 64)) {
                        new_state.color = current_state.color;
                        new_state.x_coordinate = current_state.x_coordinate;
                        new_state.y_coordinate = current_state.y_coordinate - 1;
                        search_container.push_back(new_state);
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = 'E';
                    }
                    if ((game
                                  .maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate - 1]
                                  .character
                                > 96)
                               && (game
                                     .maze[current_state.color][current_state.x_coordinate]
                                          [current_state.y_coordinate - 1]
                                     .character
                                   < 123)) {
                        new_state.color = current_state.color;
                        new_state.x_coordinate = current_state.x_coordinate;
                        new_state.y_coordinate = current_state.y_coordinate - 1;
                        search_container.push_back(new_state);
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = 'E';
                    } 
                    if ((game
                                  .maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate - 1]
                                  .character
                                == '.')
                               || (game
                                     .maze[current_state.color][current_state.x_coordinate]
                                          [current_state.y_coordinate - 1]
                                     .character
                                   == '?')
                               || (game
                                     .maze[current_state.color][current_state.x_coordinate]
                                          [current_state.y_coordinate - 1]
                                     .character
                                   == '^')
                                || (game
                                     .maze[current_state.color][current_state.x_coordinate]
                                          [current_state.y_coordinate - 1]
                                     .character
                                   == '@')) {
                        new_state.color = current_state.color;
                        new_state.x_coordinate = current_state.x_coordinate;
                        new_state.y_coordinate = current_state.y_coordinate - 1;
                        if(game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate - 1].character == '?') {
                          current_state = new_state;
                          game.solved = true;
                        } else {
                          search_container.push_back(new_state);

                        }
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].discovered = true;
                        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].coordinate_backtrace = 'E';
                    }
                }
            }
        
    }

    search_container.clear();
    
    if (game.solved) {
        search_container.push_back(current_state);

        while ((current_state.color != 0) || (current_state.x_coordinate != game.starting_x) || (current_state.y_coordinate != game.starting_y)) {
          if(game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate].coordinate_backtrace == 'S') {
            current_state.x_coordinate = current_state.x_coordinate + 1;
          } else if(game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate].coordinate_backtrace == 'W') {
            current_state.y_coordinate = current_state.y_coordinate - 1;
          } else if(game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate].coordinate_backtrace == 'N') {
            current_state.x_coordinate = current_state.x_coordinate - 1;
          } else if(game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate].coordinate_backtrace == 'E') {
            current_state.y_coordinate = current_state.y_coordinate + 1;
          } else {
            if(game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate].coordinate_backtrace == '^') {
              current_state.color = 0;
            } else {
              current_state.color = static_cast<uint8_t>(game.maze[current_state.color][current_state.x_coordinate][current_state.y_coordinate].coordinate_backtrace - 96);
            }
          }

          search_container.push_back(current_state);
        }
    } else {   // if no solution is found
        cout << "No solution.\n"
             << "Discovered:\n";
        bool is_discovered;
        for (uint32_t i = 0; i < game.total_rows; ++i) {
            for (uint32_t j = 0; j < game.total_columns; ++j) {
                is_discovered = false;
                for (uint32_t k = 0; k < static_cast<uint32_t>(game.num_colors + 1); ++k) {
                    if (game.maze[k][i][j].discovered) {
                      is_discovered = true;
                      break;
                    }
                }
                if (is_discovered) {
                    cout << game.maze[0][i][j].character;
                } else {
                    cout << "#";
                }
            }
            cout << "\n";
        }
     return 0;
    }
    

  // output list mode
  if (game.outputList) {
    while (!search_container.empty()) {
      new_state = search_container.back();
      search_container.pop_back();
      if (new_state.color == 0) {
        c = '^';
      } else {
        c = static_cast<char>(new_state.color + 96);
      }
      cout << "(" << c << ", (" << new_state.x_coordinate << ", " << new_state.y_coordinate << "))\n";
    }
  }
  if (game.outputMap) { //map output
    while (!search_container.empty()) {
      new_state = search_container.back();
      search_container.pop_back();

      if((new_state.x_coordinate == game.starting_x) && (new_state.y_coordinate == game.starting_y)) {
        if(new_state.color == 0) {
          game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].character = '@';
        } else {
          game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].character = '+';
        }
      } else if ((game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].character == '.')  || (game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].character == static_cast<char>(new_state.color + 64)) || ((game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].character == '^') && (new_state.color == 0))) {
        game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].character = '+';
      } else if (!search_container.empty() ) {
        if((new_state.x_coordinate == search_container.back().x_coordinate) && (new_state.y_coordinate == search_container.back().y_coordinate)) {
          game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].character = '%';
          game.maze[search_container.back().color][search_container.back().x_coordinate][search_container.back().y_coordinate].character = '@';
          search_container.pop_back();
        } else {
          game.maze[new_state.color][new_state.x_coordinate][new_state.y_coordinate].character = '+';
        }
      }
    }

    for (uint32_t k = 0; k < static_cast<uint32_t>(game.num_colors + 1); k++) {
      for (uint32_t i = 0; i < game.total_rows; i++) {
        for (uint32_t j = 0; j < game.total_columns; j++) {
          if ((game.maze[k][i][j].character == '^') && (k == 0)) {
            game.maze[k][i][j].character = '.';
          } else if((i == game.starting_x) && (j == game.starting_y) && game.maze[k][i][j].character != '+') {
            if(k == 0) {
              game.maze[k][i][j].character = '@';
            } 
            else {
              game.maze[k][i][j].character = '.';
            }
          } else if ((game.maze[k][i][j].character == static_cast<char>(k + 64)) && (game.maze[k][i][j].character != '@')) {
            game.maze[k][i][j].character = '.';
          }
        }
      }
    }

    for (uint32_t k = 0; k < static_cast<uint32_t>(game.num_colors + 1); ++k) {
      if (k == 0) {
        cout << "// color ^\n";
      } else {
        cout << "// color " << static_cast<char>(k + 96) << "\n";
      }
      for (uint32_t i = 0; i < game.total_rows; ++i) {
        for (uint32_t j = 0; j < game.total_columns; ++j) {
          if(game.maze[k][i][j].character == static_cast<char>(k + 96)) {
            cout << '.';
          } else {
          cout << game.maze[k][i][j].character;
          }
        }
        cout << "\n";
      }
    }  
  }

  return 0;
}
