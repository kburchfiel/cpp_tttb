/* C++ Version of Type Through the Bible

By Kenneth Burchfiel
Released under the MIT License

Link to project's GitHub page:
https://github.com/kburchfiel/cpp_tttb

Note: As with my other GitHub projects, I chose not to use
generative AI tools when creating Type Through the Bible.
I wanted to learn how to
perform these tasks in C++ (or in a C++ library) rather than
simply learn how to get an AI tool to create them.

This code also makes extensive use of the following
open-source libraries:

1. Vincent La's CSV parser (https://github.com/vincentlaucsb/csv-parser)
2. CPP-Terminal (https://github.com/jupyter-xeus/cpp-terminal)

In addition, this program uses the Catholic Public Domain Version
of the Bible that Ronald L. Conte put together. This Bible can be
found at https://sacredbible.org/catholic/ I last updated my local
copy of this Bible (whose text does get updated periodically)
around June 10, 2025.

Some relevant notes:
1. I found that, when I tried to mix std::cin and Term::cin,
certain user prompts would work fine the first time around but
then fail again. I think that this may be because the cpp-terminal
library changes default cin settings; for reference,
see https://github.com/jupyter-xeus/cpp-terminal/issues/320 .
To avoid this issue, I simply replaced all std::cin and std::cout
calls with Term::cin and Term::cout, respectively.
However, I then found that Term::cin calls didn't always work
within OSX following typing tests; therefore, I ended up updating
the game so that all input was entered within raw_mode.

2. It also appears that the cpp-terminal library requires
std::endl or std::flush at the end of Term::cout statements rather than '\n';
otherwise, output may not appear.

3. References to 'PPP3' refer to the 3rd Edition of
// Programming: Principles and Practice Using C++
// by Bjarne Stroustrup.

This game is dedicated to my wife, Allie. I am very grateful
for her patience and understanding as I worked to put it
together! My multiplayer gameplay sessions with her (yes, she
was kind enough to play it with me) also helped me refine the
code and improve the OSX release.

Blessed Carlo Acutis, pray for us!


*/

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

#include "cpp-terminal/color.hpp"
#include "cpp-terminal/cursor.hpp"
#include "cpp-terminal/exception.hpp"
#include "cpp-terminal/input.hpp"
#include "cpp-terminal/iostream.hpp"
#include "cpp-terminal/key.hpp"
#include "cpp-terminal/options.hpp"
#include "cpp-terminal/screen.hpp"
#include "cpp-terminal/terminal.hpp"
#include "cpp-terminal/tty.hpp"
#include "cpp-terminal/version.hpp"
#include "csv.hpp"
using namespace csv;

// Storing codes that correspond to 16 different background
// colors within the ANSI escape code system:
// This code is based on the hello world example at
// https://github.com/jupyter-xeus/cpp-terminal and on the
// ANSI escape sequence color reference at
// https://en.wikipedia.org/wiki/ANSI_escape_code#3-bit_and_4-bit .
std::vector<std::string> background_color_codes{/*Bright Red*/ "101",
                                                /*Bright Green*/ "102",
                                                /*Bright Blue*/ "104",
                                                /*Bright Yellow*/ "103",
                                                /*Bright Magenta*/ "105",
                                                /*Bright Cyan*/ "106",
                                                /*Bright Black*/ "100",
                                                /*Bright White*/ "107",
                                                /*Red*/ "41",
                                                /*Green*/ "42",
                                                /*Blue*/ "44",
                                                /*Yellow*/ "43",
                                                /*Magenta*/ "45",
                                                /*Cyan*/ "46",
                                                /*Black*/ "40",
                                                /*White*/ "47"};

// Specifying prefixes and suffixes that will precede and proceed
// these background color codes, respectively:
std::string background_color_prefix = "\033[37;"; //
// The ANSI escape code for white foreground text.
std::string background_color_suffix = "m    \033[0m";
// Four spaces
// (which will give the background color space to appear)
// followed by an ANSI escape code that restores default color
// settings)

std::string all_verses_typed_message = "All verses have already \
been typed at least once! try another choice (such as i for a \
non-marathon-mode session or s for a marathon-mode session).";

std::string verses_file_path = "../Files/CPDB_for_TTTB.csv";
std::string autosaved_verses_file_path = "../Files/\
autosaved_CPDB_for_TTTB.csv";

// Defining colors that represent correct and incorrect output:
// (This code is based on the Hello World example for
// cpp-terminal at https://github.com/jupyter-xeus/cpp-terminal .)
// A full list of color names can be found at
// https://jupyter-xeus.github.io/cpp-terminal/cpp-terminal_Manual.pdf .

std::string correct_output_color_code =
    Term::color_fg(Term::Color::Name::Green);
std::string incorrect_output_color_code =
    Term::color_fg(Term::Color::Name::Magenta); // Switched from Red to Magenta
// to make the game more accessible to colorblind players.
// (Special thanks to David Nichols for his excellent
// colorblindness reference at
// https://davidmathlogic.com/colorblind -- and for suggesting
// magenta as an alternative to green.)
std::string default_output_color_code =
    Term::color_fg(Term::Color::Name::Default);
std::string print_color_code = default_output_color_code; // This
// variable will get updated within typing tests to reflect either
// correct or incorrect output.

/* Defining a struct that can represent each row
of CPDB_for_TTTB.csv (the .csv file containing all
Bible verses:) */

struct Verse_Row {
  int verse_id;
  std::string ot_nt;
  std::string book;
  int book_num;
  std::string chapter_num; // This is actually a string, since
  // some values are 'P' (for 'prologue').
  int verse_num;
  std::string verse_code;
  std::string verse;
  int characters;
  int tests; // Will store how many times the user has typed
  // a given verse.
  double best_wpm;
};

// Defining a struct that can store relevant test result data:
// (Since this data will be used to populate all rows within
// the test results CSV file, it will need to include all
// of the rows within that file as well. However, not all of
// the attributes stored here will actually end up within
// that file.)
struct Test_Result_Row {
  long test_number; // Stores the total number of tests the player
  // has completed so far.
  long session_number; // Stores the total number of
  // sessions the player has completed so far (e.g. the
  // total number of times the player has launched a
  // single-player game).
  int within_session_test_number; // Stores the total number of
  // tests completed by the player during the current session.
  long unix_test_start_time;
  std::string local_test_start_time;
  long unix_test_end_time;
  std::string local_test_end_time;
  int verse_id;
  std::string verse_code;
  std::string verse; // In rare occasions, corrections might be
  // made to a given verse. (For instance, I replaced double
  // spaces in the original CPDB text with single spaces.)
  // Therefore, it will be helpful to keep a record of the verse
  // that the user actually typed, even though this will make
  // the test results file considerably larger.
  int characters; // Similarly, this length could potentially
  // change between tests (e.g. if extra spaces get removed from
  // a verse). Although this value can be determined from the
  // verse itself, including it here won't take up too much
  // extra space.
  double wpm;
  double test_seconds;
  double error_rate;
  double error_and_backspace_rate;
  // This struct will be expanded to include accuracy data,
  // the time the test was started and finished, and possibly other
  // items also.
  int marathon_mode; // Tracks whether or not marathon mode was
  // active during a test.
  // Entries for player info and other tags that the player
  // can use to store custom information (e.g. about
  // what keyboard is being used, how much sleep he/she got
  // the night before, etc.)
  // These will be initialized as empty strings because, as
  // optional fields, there may not be any data entered for
  // them during the typing test.
  std::string player = "";
  std::string mode = ""; // Can be either SP for single player
  // or MP for multiplayer.
  std::string tag_1 = "";
  std::string tag_2 = "";
  std::string tag_3 = "";
  std::string notes = "";
};

// Defining a struct that can store word-level WPM information:
// (This struct will be used to keep track of WPM results for
// individual words within tests.)

struct Word_Result_Row {
  long test_number; // Storing this value within
  // Word_Result_Row will make it easier to link these results
  // to their respective tests. (Indeed, this value will allow
  // us to avoid having to store player names, tags, etc.
  // within word results rows, as we can retrieve those by merging
  // their respective test result columns into our word results
  // table using test_number as a key.
  long unix_test_start_time; // Added as a fallback merge
  // key in case, somehow,
  // word-result and test-result test numbers get unsynced.
  // (It's possible, but high unlikely, that two test results
  // will have the same test start time; thus, the test_number
  // value should still be the best merge key.
  std::string word = "";
  // Initializing certain variables as -1 or -1.0 will make it
  // easier to identify cases in which they weren't updated.
  // I could try this approach with other structs as well.
  long last_character_index = -1;
  // The starting character could also be stored, but since this
  // value will be used as a map key, it would be redundant to
  // include it here also.
  long word_length = -1; // an int would likely work fine here.
  double wpm = -1.0;
  double test_seconds = -1.0;
  double error_rate = -1.0;
  double error_and_backspace_rate = -1.0;
};

// Defining a set of configuration settings that can be updated
// by the user during the game:

struct Game_Config {
  std::string player = "";
  std::string tag_1 = "";
  std::string tag_2 = "";
  std::string tag_3 = "";
  std::string notes = "";
  std::string mode = "";
};

// Defining a struct that will allow me to keep track of
// the amount of time, in microseconds, the game needs to process
// individual keypresses:
// (Note: I've since commented this code out, as it was
// meant for testing/debugging purposes.)
// struct Keypress_Processing_Time_Row
// {
//     long test_number;
//     int keypress_number;
//     long processing_time_in_microseconds;
// };

std::string cooked_input_within_raw_mode(
    std::string prompt = "",
    bool multiline = false) { /* This function aims to provide 'cooked-like'
                             input within a raw mode cpp-terminal session. It
                             does so by allowing users to add to a string until
                             they press enter. If the input may be more than one
                             line long, set multiline to true. The function will
                             then clear the console and display only the prompt,
                             thus making multi-line input easier to handle.*/

  int starting_result_row = 2; // Defining the row at which
  // the player's input should be typed. (Will only be used
  // for multiline results, as otherwise, it would likely
  // overwrite existing information on the screen.)
  std::string user_string;
  // Defining a string that will allow the cursor to get moved
  // directly below the prompt:
  std::string cursor_reposition_code =
      Term::cursor_move(starting_result_row, 1);

  if (multiline == true) {
    int prompt_length = prompt.size();
    // Determining on which row to begin printing the user's
    // input: (See similar code within run_test() for more
    // details.)
    Term::Screen term_size{Term::screen_size()};
    starting_result_row = (prompt_length - 1) / (term_size.columns()) + 2;

    // Clearing the console and displaying the prompt:
    // (Note that, unlike within run_test(), clear() isn't
    // called here, as the user might want to be able to scroll
    // up to see previous input.
    Term::cout << Term::clear_screen() << Term::cursor_move(1, 1) << prompt
               << std::endl;

    // Calling cursor_move to determine the cursor reposition
    // code that will bring the cursor right under the verse
    // after each keypress:

    cursor_reposition_code = Term::cursor_move(starting_result_row, 1);
  }

  else

  { // Printing the prompt, then adding two newlines (including that
    // printed by std::endl) to give
    // the player more space to enter his/her response:
    Term::cout << prompt << "\n" << std::endl;
    // If multiline isn't active, the cursor reposition code
    // will be set as "\r", the carriage return call, which
    // will move the cursor back to the start of the line.

    cursor_reposition_code = "\r";
  }
  std::string keyname;
  while (keyname != "Enter") // May also need to add 'Return'
  // or 'Ctrl+J' here also for OSX compatibility)
  {
    Term::Event event = Term::read_event();
    switch (event.type()) {
    case Term::Event::Type::Key: {
      Term::Key key(event);
      std::string char_to_add = "";
      keyname = key.name();
      if (keyname == "Space") {
        char_to_add = " ";
      } else if (keyname == "Backspace") // We'll need to remove the
      // last character from our string.
      {
        user_string = user_string.substr(0, user_string.length() - 1);
      } else if (keyname == "Enter") {
        break;
      } else {
        char_to_add = keyname;
      }
      user_string += char_to_add;
      // See run_test documentation for more information about
      // "\033[J". This code repositions the cursor (either
      // to the beginning of the current line if multiline
      // is false or to the beginning of the line below the
      // prompt if multiline is true); clears out everything
      // past this point; then shows what the user has entered
      // so far.
      Term::cout << cursor_reposition_code << "\033[J" << user_string
                 << std::flush; // Using std::flush rather
      // than std::endl so that we can begin our new
      // entry from the same line (which will be useful in
      // non-multiline mode).
    }
    default: {
      break;
    }
    }
  }

  // For debugging:
  //  Term::cout << "The string you entered was:\n"
  //  << user_string << std::endl;
  return std::move(user_string);
}

std::string get_single_keypress(std::vector<std::string> valid_keypresses = {})
/* This function retrieves a single keypress, then responds
immediately to it.
The valid_keypresses argument allows the caller to specify
which keypresses should be considered valid entries. If
the default option is kept, any keypress will be considered
valid. */

{
  std::string single_key;
  bool valid_keypress_entered = false;
  while (valid_keypress_entered == false) {
    Term::Event event = Term::read_event();
    switch (event.type()) {
    case Term::Event::Type::Key: {
      Term::Key key(event);
      single_key = key.name();
      if (valid_keypresses.size() > 0) {
        for (std::string valid_keypress : valid_keypresses) {
          if (single_key == valid_keypress) {
            valid_keypress_entered = true;
          }
        }
        if (valid_keypress_entered == false) {
          Term::cout << single_key << " isn't a valid entry. Please \
try again." << std::endl;
        }
      } else // In this case, we'll assume the keypress
      // to be valid.
      {
        valid_keypress_entered = true;
      }
    }
    default: {
      break;
    }
    }
  }
  return single_key;
}

int select_verse_id(const std::vector<Verse_Row> &vrv,
                    const int &last_verse_offset = 0) {
  /* This function allows the player to select a certain verse ID to
  type. It also checks for errors in order to (hopefully!) prevent
  the game from crashing.

  last_verse_offset was added in to allow the last valid verse ID
  to be reduced within multiplayer games in which more than one
  verse is typed. Without this offset, a multiplayer game could end
  up attempting to access verses beyond the final verse, which would
  likely result in a crash.


  Note: I had originally set the value typed by the user to an
  integer, then checked to see whether cin was valid; however,
  this created a strange (to me) bug in which the function would
  work fine the first time, but not during subsequent uses.
  I did include cin.clear() and cin.ignore() calls, but to no avail.
  (This was
  most likely due to the use of std::cout following Term::cout, which
  may have caused the former not to work correctly.)
  Therefore, I decided to read in a string, then use a try/except
  block to handle it. This approach is working much better,
  thankfully!*/
  std::string id_response_as_str = "";
  while (true) {
    Term::cout << "Type in the ID of the verse that you would \
like to type, then hit Enter. This ID can be found in the first \
column of the CPDB_for_TTTB.csv file. To exit out of this option, \
type -1 followed by Enter."
               << std::endl;
    // Checking for a valid response:
    id_response_as_str = cooked_input_within_raw_mode();
    try {
      int id_response_as_int = std::stoi(id_response_as_str);

      if (id_response_as_int == -1 || id_response_as_int == -2)
      // Added in -2 to support random-verse selection within
      // multiplayer games
      {
        // Term::cout << "Never mind, then!" << std::endl;
        return id_response_as_int;
      }

      else if ((id_response_as_int >= 1) &&
               (id_response_as_int <= (vrv.size() - last_verse_offset)))
      // Subtracting 1 from id_response_as_int will get us the index
      // position of that verse (as the index of each verse is one less
      // than its ID).
      {
        return (id_response_as_int - 1);
      } else {
        Term::cout << "\nThat is not a valid ID. \
Please try again." << std::endl;
        // return -1;
      }
    }

    catch (...) {
      Term::cout << "Your input was invalid. \
Please try again."
                 << std::endl;
      // return -1;
    }
  }
}

std::map<long, Word_Result_Row> gen_word_result_map(const std::string &verse)
/*This function will go through each character within the verse
passed to it in order to identify all words within the verse;
their starting and ending characters; and their lengths. This
information will then be stored within a map of Word_Result_Row objects
that the typing test code can access in order to calculate word-level
WPM data. */

{
  // Initializing several variables here so that they can get
  // utilized within the following loops:
  int first_character_index = -1;
  std::string newword = "";
  std::map<long, Word_Result_Row> word_map;

  // Checking for the first character within the verse that starts
  // a word:
  for (int i = 0; i < verse.size(); i++) {
    if (isalnum(verse[i]) != 0)
      first_character_index = i;
    newword = verse[i];
    break;
  }

  // Term::cout << "Current values of first_character_index \
// and newword: " << first_character_index << " " << newword << std::endl;

  // Now that we know where the first character that starts a word
  // is located,
  // we can continue to retrieve the other characters (starting
  // from the character following this first character).

  for (int i = first_character_index + 1; i < verse.size(); i++)

  {
    /*If a character is alphanumeric, and the one prior to it
    was not, we'll consider this to be the start of a new word.
     Note that isalnum returns either a 0 or non-0 number
     (see https://en.cppreference.com/w/cpp/string/byte/isalnum;
     in my case, it was 8), which is why I'm checking for
     0 and non-0 in my if statement. This code could likely
     be simplified, however.*/
    if ((isalnum(verse[i]) != 0) && (isalnum(verse[i - 1]) == 0)) {
      first_character_index = i;
      newword = verse[i]; // I had previously tried to
      // create a Word_Result_Row class here and assign verse[i] to
      // its .word attribute, but this failed to work correctly.
      // Term::cout << "Starting character info: "
      // << i << " " << verse[i] << std::endl;
    }

    else if (((isalnum(verse[i]) == 0) && (isalnum(verse[i - 1]) != 0)) ||
             ((isalnum(verse[i]) != 0) && (i == (verse.size() - 1))))
    /*In this case, either verse[i-1] marks the end of a word,
    or the final character of the verse is part of a word.
    For both of these situations, we should go ahead and add
    this word to a new Word_Result_Row object, then add that object
    to our word map (with the initial character as the key). */
    {
      int last_character_index = i - 1;
      // Changing this value to i if the second condition
      // (e.g. the final word within the verse is a letter) is true:

      if ((isalnum(verse[i]) != 0) && (i == (verse.size() - 1))) {
        last_character_index = i;
        // In this case, we'll need to add this final character
        // to newword also.
        newword += verse[i];
      }
      // Creating a new Word_Result_Row object that can store
      // various attributes about this word:
      // (Other attributes of this row will get filled in
      // within run_test().)
      Word_Result_Row wr;
      wr.word = newword;
      wr.word_length = newword.size();
      wr.last_character_index = last_character_index;
      word_map[first_character_index] = wr;
      // Term::cout << "Ending character info: " << i << " "
      // << verse[i-1] << std::endl;
    }

    else if (isalnum(verse[i]) != 0)
    // In this case, we're in the middle of constructing a word,
    // so we'll go ahead and add this alphanumeric character
    // to that word.
    {
      newword += verse[i];
    }
  }

  // Printing out all initial characters, ending characters,
  // and words within the map for debugging purposes:

  // for (auto const& [starting_character, word_result] : word_map)
  // {Term::cout << word_result.word << ": characters " <<
  // starting_character << " to " <<
  // word_result.last_character_index
  // << " (" << word_result.word_length << " characters long)"
  // << std::endl;}

  // Moving word_map in order to avoid an unnecessary copy:
  return std::move(word_map);
};

bool run_test(Verse_Row &verse_row, std::vector<Test_Result_Row> &trrv,
              std::vector<Word_Result_Row> &wrrv, const bool &marathon_mode,
              const std::string &player, const std::string &mode,
              const std::string &tag_1, const std::string &tag_2,
              const std::string &tag_3, const std::string &notes,
              long &test_number, long &session_number,
              int &within_session_test_number, const bool allow_quitting,
              // std::vector<Keypress_Processing_Time_Row> &kptrv,
              const std::string &within_test_update_message,
              const std::string &post_entry_message)
/* This function allows the player to complete a single typing test.
It then updates the verse_row, trrv, and wrrv vectors with the results
of that test.
*/
{
  /* Some of the following code was based on the documentation at
  https://github.com/jupyter-xeus/cpp-terminal/blob/
  master/examples/keys.cpp .*/

  // int keypress_counter = 0; // Currently, this value is only
  //  being used to keep track of keypress processing times.

  // Creating a Keypress Processing Time Row vector that will
  // store results while a test is in progress:
  // (Once the test is completed, additional information will
  // get added to these tests, and they will then get stored
  // within kptrv. This approach will allow the main
  // keypress processing time vector to remain unmodified
  // until the test has been complete. (Otherwise, we might end
  // up assigning incorrect test result names to keypresses
  // in cases when players quit tests early.)
  // std::vector<Keypress_Processing_Time_Row> local_kptrv;

  // Calling gen_word_result_map() to create a map that contains
  // information about each word within the verse:
  // (This will prove useful when calculating word-level WPM data.)

  std::map<long, Word_Result_Row> word_map =
      gen_word_result_map(verse_row.verse);

  // Initializing several variables that will be used later in
  // this function:
  // Initializing counters that will keep track of the number of
  // errors the user made:
  // (One of these counters won't count backspaces as errors;
  // the other will.)
  long error_counter = 0;
  long backspace_counter = 0;
  long word_error_counter = 0;
  long word_backspace_counter = 0;

  // Initializing a string to which characters typed by the user
  //  will be added: (This will allow us to print the entire portion
  //  of the verse that the user has typed so far, rather than just
  //  the most recent character.)
  std::string user_string = "";

  bool exit_test = false;
  bool completed_test = true;
  int last_character_index = -1;
  // For debugging:
  std::string last_character_index_as_string = "";
  // std::string word_timing_note = ""; // for debugging
  int latest_first_character_index = -1;
  int first_character_index = -1;
  long word_length = 0;
  auto word_start_time = std::chrono::high_resolution_clock::now();

  Term::Cursor cursor{Term::cursor_position()};

  Term::Screen term_size{Term::screen_size()};

  // Clearing the console and displaying the verse to type:
  // In order to make the transition from the following dialog
  // to the actual test less jarring, I chose to place the verse
  // at the top of the screen, exactly where it will appear
  // during the test itself. I also placed the post-entry message
  // at the same place that it will appear when a marathon mode
  // is active.

  // Note: clear_screen() hides content within the current
  // window by scrolling down until the window is blank;
  // clear() removes content further up in the terminal that is
  // now out of view. Thus, printing clear_screen(), followed by
  // clear() and a command to move the cursor to the top left,
  // prevents the terminal from filling up with previous entries
  // that no longer
  // need to be saved in memory.
  // (Note that, without the
  // inclusion of clear(), a new screen would be stored in
  // memory for all, or almost all *keypresses* during typing
  // tests--which I imagine could cause all sorts of
  // memory- or performance-related issues, at least on lower-powered devices.

  // I found clear() within the cpp-terminal documentation;
  // you can search for it by looking for the text [3J (which
  // is part of a particular 'Erase in Display' ANSI erase
  // sequence; see https://en.wikipedia.org/wiki/ANSI_escape_code#SGR
  // and Goran's AskUbuntu response at
  // https://askubuntu.com/a/473770/1685413 .

  Term::cout << Term::clear_screen() << Term::terminal.clear()
             << Term::cursor_move(1, 1) << verse_row.verse << "\n"
             << post_entry_message << std::endl;

  // Determining where to position the cursor after each keypress:
  // The best option here will be to position it at the leftmost
  // column just under the verse. That way, we won't need to
  // resend the verse to the terminal before each keypress,
  // but we'll also be able to easily account for multiple-line
  // verses, backspaces, and other special cases.
  // To figure out which row belongs just under the verse, we
  // can divide the length of the verse minus one by
  // term_size.columns(), then add 2 to the result. (This value
  // should always be an integer, since we're dividing one
  // integer by another; thus, the remainder of this division
  // operation will be dropped by default. Subtracting 1 from
  // the length of the verse will prevent an extra row from
  // getting added if the length of the verse is an exact multiple
  // of the width of the terminal.)

  int starting_result_row =
      (verse_row.characters - 1) / (term_size.columns()) + 2;

  // Calling cursor_move to determine the cursor reposition
  // code that will bring the cursor right under the verse
  // after each keypress:

  std::string cursor_reposition_code =
      Term::cursor_move(starting_result_row, 1);

  /*
  Note that, for cursor_move to have an effect, we need to pass
  its output (a string) to Term::cout. Also note that using
  std::cout in place of Term::cout may not work.
  The kilo.cpp example (available at
  https://github.com/jupyter-xeus/cpp-terminal/blob/master/examples/kilo.cpp
  and the cursor.cpp source code at
  https://github.com/jupyter-xeus/cpp-terminal/blob/master/cpp-terminal/cursor.cpp
  helped me recognize all this.)
  I've also found that ending Term::cout lines with "\n" may
  not work; instead, it may be necessary to use std::endl or std::flush.
  */

  if (marathon_mode == false) // The following prompt should be skipped
  // within marathon mode, thus allowing users to go directly
  // into the typing test.
  {
    Term::cout << "\n"
               << within_test_update_message << "\nYour \
next verse to type ("
               << verse_row.verse_code << ") is shown above. This verse is "
               << verse_row.characters << " characters long.\nPress \
the space bar to begin the typing test and 'e' to cancel it."
               << std::endl;

    // The following while statement allows the user to begin the
    // test immediately after pressing *only* the space bar.
    // It also lets the player cancel the test by
    // pressing 'e.'
    // The verse will still be present at the
    // top left of the terminal so that
    // the user can reference it when beginning the test.
    // the top left so that its location won't change between this
    // section of the script and the following while loop.)

    std::string player_response = get_single_keypress({"Space", "e"});
    if (player_response == "e") {
      exit_test = true;
      completed_test = false;
    }
  }

  // Determining the start time of the test in system clock form:
  // (Note: Linux allowed the start_time created by
  // std::chrono::high_resolution_clock::now() to get converted
  // to unix_test_start_time, but Windows didn't--hence my addition
  // of code to derive unix_test_start_time from
  // system_clock instead.
  // This code was based on
  // https://en.cppreference.com/w/cpp/chrono/system_clock/to_time_t.html
  // and https://en.cppreference.com/w/cpp/chrono/system_clock/now.html .
  std::time_t unix_test_start_time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  // Starting our timing clock:
  // (This code was based on p. 1010 of The C++ Programming Language,
  // 4th edition.)
  auto start_time = std::chrono::high_resolution_clock::now();

  /* Checking to see whether the first character begins
  one of the words in word_map: (This will generally, but not
  always be the case.)
  If so, we'll need to begin word-timing-related
  processes immediately.
  */

  if (word_map.contains(0)) {
    // Updating latest_first_character_index:
    latest_first_character_index = 0;
    last_character_index = word_map[0].last_character_index;
    word_start_time = start_time;

    word_length = word_map[0].word_length;
    last_character_index_as_string = std::to_string(last_character_index);
    // word_error_counter and word_backspace_counter
    // were already initialized to 0 earlier within
    // this function, so we don't
    // need to re-initialize them as 0 here.
    // word_timing_note = "the first letter within the verse \
// began a word. its corresponding ending character index \
// is: "+last_character_index_as_string;
  }

  // Users may wish to exit a given test before completing it,
  // so we should accommodate that choice.
  // However, in that case, this boolean will be set to 'false' so that
  // we don't mistake the exited test for a completed one.
  while ((user_string != verse_row.verse) && (exit_test == false)) {
    Term::Event event = Term::read_event();
    switch (event.type()) {
    case Term::Event::Type::Key: {
      Term::Key key(event);
      // keypress_counter++;
      //  Creating a timer following this keypress:
      //  (This will be useful for timing how long it took the user
      //  to type each individual word.)
      auto keypress_time = std::chrono::high_resolution_clock::now();

      std::string char_to_add = ""; // This default setting will
      // be used in certain special keypress cases (including Backspace)
      std::string keyname = key.name();

      if (keyname == "Space") {
        char_to_add = " ";
      } else if ((keyname == "Ctrl+C") && (allow_quitting == true)) {
        exit_test = true;
        completed_test = false;
      }

      else if (keyname == "Backspace") // We'll need to remove the
      // last character from our string.
      {
        user_string = user_string.substr(0, user_string.length() - 1);
        backspace_counter++; // Since the user pressed backspace,
        // we'll need to increment this counter by 1 so that this
        // keypress can get reflected within our error_and_backspace_counter
        // value.
        word_backspace_counter++;
      }
      // For documentation on substr, see
      // https://cppscripts.com/string-slicing-cpp

      // Defining key combinations that will allow entire
      // words to be deleted:
      else if ((keyname == "Alt+Del") || (keyname == "Del") ||
               (keyname == "Alt+Backspace"))
      // Alt+Del, Del, and Alt+Backspace were added in for
      // Linux, OSX, and Windows, respectively; thankfully,
      // they don't seem to conflict with one another.
      // I found these key names by experimenting with the
      // executable version of the keys.cpp example within
      // the cpp-terminal library (available at
      // https://github.com/jupyter-xeus/cpp-terminal/blob/master/examples/keys.cpp
      // ). Note that Alt+Del is generated in Linux by pressing Alt+Backspace
      // (at least when using Linux Mint on my Gigabyte Aorus laptop); Del is
      // generated in OSX by pressing Function + Delete; and Alt+Backspace is
      // generated in Windows by pressing Alt+Backspace. I would have liked to
      // set Ctrl + Backspace as an option, but this key combination simply
      // produced 'Backspace' for me on Windows and Linux. Ctrl + Backspace
      // (which seemed to be interpreted as just Backspace by the cpp-terminal
      // library). The following code will remove all characters from the end of
      // the string up to (but not including) the most recent space. However, if
      // the last character in the string happens to be a space as well, the
      // loop will skip over it and search instead for the second-to-last space.
      // (That way, repeated Alt+Del entries can successfully remove multiple
      // words.) Note: the cpp-terminal code interpreted the combination of the
      // Alt and Backspace keys, at least on Linux, as Alt + Delete. Note: Alt +
      // Delete was interpreted as a regular Backspace entry on the Mac on which
      // I tested TTTB; therefore, I added Del (which could be produced via Fn +
      // Delete on that Mac) as an option.)
      {
        backspace_counter++;
        word_backspace_counter++;
        for (int j = user_string.length() - 1; j >= 0; --j) {
          if ((isspace(user_string[j])) && (j != (user_string.length() - 1)))
          // if (isspace(user_string[j]))
          /* Truncating the response so that it only extends up to
          the latest space (thus removing all of the characters
          proceeding that space): */
          {
            user_string = user_string.substr(0, j + 1);
            break;
          }
          if (j == 0) // In this case, we made it to
          // the beginning of the string without finding
          // any space. Therefore, we'll go ahead and
          // delete the entire sequence that the player
          // has typed so far. (This inclusion allows
          // Alt + Delete to still work when the player
          // hasn't yet finished his/her first word.)
          {
            user_string = "";
            break;
          }
        }
      }

      else {
        char_to_add = keyname;
      }

      user_string += char_to_add;
      /* Determining how to color the output: (If the output is correct
      so far, it will be colored green; if there is a mistake, it will
      instead be colored red.*/
      if (user_string == verse_row.verse.substr(0, user_string.length()))
      // Checking to see whether we should start or end our word timer:
      { // Checking to see whether we should begin timing a new word:
        // This new word's index position will be one greater than
        // the current index position--but, conveniently, user_string.length()
        // allows us to access that position. And we'll want to
        // start the timing right before the first letter of the word so that
        // the time that it takes that letter to get typed can also get
        // included in the overall timing for the word.
        // The code also checks to see whether the length of the user's entry
        // is greater than the most recent first_character_index so that
        // the user can't 'get out of' or restart the current word timer by
        // going back to the start of the current word or a previous word.
        if ((word_map.contains(user_string.length())) &&
            (long(user_string.length()) > latest_first_character_index))
        // Note that string.length() needs to be cast to a long or int
        // in order to get compared with latest_first_character_index.

        { // Updating latest_first_character_index:
          latest_first_character_index = user_string.length();
          last_character_index =
              word_map[latest_first_character_index].last_character_index;
          word_start_time = keypress_time;
          // Resetting word-specific error and backspace counters:
          word_error_counter = 0;
          word_backspace_counter = 0;

          word_length = word_map[latest_first_character_index].word_length;
          last_character_index_as_string = std::to_string(last_character_index);
          // word_timing_note = "the next letter will start a word. \
// its corresponding ending character index \
// is: "+last_character_index_as_string;
        }
        if (user_string.length() - 1 ==
            last_character_index) { // In this case, we've made it to the end of
                                    // the word whose
          // starting character we approached earlier. Thus, we can
          // stop our word timer and create a new Word_Result_Row object
          // with our output.
          auto word_end_time = keypress_time;
          auto word_seconds =
              std::chrono::duration<double>(word_end_time - word_start_time)
                  .count();
          double word_wpm = (word_length / word_seconds) * 12;
          int word_error_and_backspace_counter =
              word_error_counter + word_backspace_counter;
          double word_error_rate =
              (word_error_counter / static_cast<double>(word_length));
          double word_error_and_backspace_rate =
              (word_error_and_backspace_counter /
               static_cast<double>(word_length));

          // word_timing_note = "the end of the word has been \
// reached. Ending character: "+last_character_index_as_string + " \
// word length: "+std::to_string(word_length) + " word \
// duration: "+ std::to_string(word_seconds) + " WPM: " + std::to_string(
          // word_wpm);
          // Note that, once we arrive at the next word's starting
          // character, certain variables (like last_character_index)
          // will get replaced with new versions.

          // Storing these details within word_map: (We can use
          // latest_first_character_index as a key here.)
          // Other elements of each word_row object that
          // don't need to be computed here (such as
          // test numbers)
          // will get added in after the test completes,
          // thus reducing the amount of
          // processing time needed to keep track of
          // word results within the race.

          word_map[latest_first_character_index].wpm = word_wpm;
          word_map[latest_first_character_index].test_seconds = word_seconds;
          word_map[latest_first_character_index].error_rate = word_error_rate;
          word_map[latest_first_character_index].error_and_backspace_rate =
              word_error_and_backspace_rate;
        }

        print_color_code = correct_output_color_code;
      } else // The user made a mistake, as signified by the fact that
      // this string doesn't match the initial section of the verse that
      // has the same length.
      {
        print_color_code = incorrect_output_color_code;
        if ((keyname != "Backspace") && (keyname != "Alt+Del")) {
          // In this case, we'll increment our main error
          // counter, as the user did not press Backspace.
          // (This code may need to be updated to also address
          // the use of Ctrl+Backspace, though on my Linux system,
          // that combination also produces the name 'Backspace.')
          error_counter++;
          word_error_counter++;
        }
      }
      /* To print the output, we'll first move the cursor back
      to the starting row for the result, then clear all text
      on and after that row. Next, we'll print the user's
      response so far. (This approach greatly simplifies our
      code, as if we tried to print just the most recent
      character, we could encounter issues when printing files
      of multiple lines or deleting one or more characters.
      It may seem 'wasteful' to print all the characters of
      the response anew each time, but it prevents us from
      having to deal with various edge cases.

      \033[J is an 'erase in display' ANSI escape code
      that clears out all text following the cursor.
      (I used the ANSI escape code Wikipedia page at
      https://en.wikipedia.org/wiki/ANSI_escape_code and the
      Hello World! demo for cpp-terminal at
      https://en.wikipedia.org/wiki/ANSI_escape_code to figure
      out what to enter here. I don't think the cpp-terminal
      library has a function for this code yet, but this approach
      works fine also.
      Also note that functions like 'cursor_move()' also make
      use of escape codes; see
      https://jupyter-xeus.github.io/cpp-terminal/cursor_8cpp_source.html
      for examples of the original codes underlying this and
      related functions.

      Note: an earlier version of this code moved the cursor
      to the top left of the terminal; cleared out all of
      the content; and then rewrote both the full verse
      and the user's response. However, this caused the player's
      response to flicker within Gnome-Terminal on Linux
      (but not Console or Hyper). Once I updated the code
      to only print the response, not the verse, this issue
      went away.
      */

      Term::cout << cursor_reposition_code << "\033[J" << print_color_code
                 << user_string << default_output_color_code
                 << post_entry_message << std::endl;

      auto processing_end_time = std::chrono::high_resolution_clock::now();

      //// Seeing how long it took the code to process the
      //// player's keypress:
      //// This code was based on the example found at
      //// https://en.cppreference.com/w/cpp/chrono/duration/duration_cast.html
      ///.
      // auto processing_microseconds = std::chrono::duration<
      // double, std::micro>(
      //     processing_end_time - keypress_time).count();

      // The following line shows the latest keypress
      // processing time value directly below the player's
      // response. This data will also be available for
      // view within a local .csv file (as long as that code
      // hasn't since been commented out).
      // Term::cout << processing_microseconds << std::endl;

      // Keypress_Processing_Time_Row kptr;
      // kptr.keypress_number = keypress_counter;
      // kptr.processing_time_in_microseconds = (
      // processing_microseconds);
      // local_kptrv.push_back(kptr);

      // Older variants of this code that are slightly less
      // (or potentially much less) efficient:

      // Term::cout << Term::cursor_move(
      //     1, 1) << "\033[J" <<
      // verse_row.verse << std::endl <<
      // Term::color_fg(print_color) << user_string <<
      // color_fg(Term::Color::Name::Default) << std::endl;

      // Term::cout << Term::clear_screen() <<
      // // Term::terminal.clear() <<
      // Term::cursor_move(
      //     1, 1)
      //            << verse_row.verse << std::endl
      //            << Term::color_fg(print_color) << user_string
      //            << color_fg(Term::Color::Name::Default)
      //            << std::endl;
      break;
    }
    default:
      break;
    }
  }

  // Ending our timing clock and calculating our WPM:
  // (This code was based in part on the examples found at
  // https://en.cppreference.com/w/cpp/chrono/duration .)
  auto end_time = std::chrono::high_resolution_clock::now();
  std::time_t unix_test_end_time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  auto test_seconds =
      std::chrono::duration<double>(end_time - start_time).count();
  /* The following time variable initialization code was based in part
  on https://en.cppreference.com/w/cpp/chrono/system_clock/now.html . */

  long unix_test_start_time_as_long = long(unix_test_start_time);
  // Creating a string version of this timestamp that shows
  // the user's local time:
  // The following formatting code was based on the examples
  // at
  // https://en.cppreference.com/w/cpp/chrono/c/strftime
  // I could also have used std::format(), but
  // this isn't available within older C++ implementations.
  // (See https://en.cppreference.com/w/cpp/chrono/
  // system_clock/formatter) for more information on that option.)
  char start_strftime_container[25];
  std::strftime(start_strftime_container, 25, "%FT%T%z",
                std::localtime(&unix_test_start_time));
  std::string local_start_time_string = start_strftime_container;
  // %F represents YYYY-MM-DD; %T represents
  // HH-MM-SS; and %z represents the local UTC offset
  // (e.g. -0400 for Eastern Daylight Time in the US).
  // (The T in between %F and %T helps differentiate
  // between these two components.)
  // Here's an example of what local_time_string might
  // look like:
  // 2025-06-12T21:49:33-0400
  // (This string is 24 characters long. I found that
  // I needed to set strftime_container as one greater
  // than this length in order for it to fit the entire
  // timestamp.

  // Performing similar steps on the end-of-test timestamp:

  long unix_test_end_time_as_long = long(unix_test_end_time);
  char end_strftime_container[25];
  std::strftime(end_strftime_container, 25, "%FT%T%z",
                std::localtime(&unix_test_end_time));
  std::string local_end_time_string = end_strftime_container;

  double wpm = (verse_row.characters / test_seconds) * 12; /* To calculate WPM,
            we begin with characters per second, then multiply by 60 (to go
            from seconds to minutes) and divide by 5 (to go from characters
            to words using the latter's standard definition).*/

  /* Calculating error rates:
  These will be equal to the number of errors divided by
  the number of verses. note that it's possible for
  the rate to be above 100%. Another option would be to
  divide the number of erroneous keypresses by the number
  of keypresses, but this approach is a bit simpler.
  (Calculating these rates as (verse_length -
  error_count)/verse_length) wouldn't be ideal, as we could then
  end up with a negative number, which would be confusing.
  */

  int error_and_backspace_counter = error_counter + backspace_counter;

  // Term::cout << "Error counter, backspace counter, error and \
// backspace counter, and verse length: " << error_counter <<
  // backspace_counter << error_and_backspace_counter << verse_length;

  // Casting verse_length to a double ensures that error_rate itself
  // will be a double, not an int (which would likely result in
  // an incorrect calculation).
  double error_rate =
      (error_counter / static_cast<double>(verse_row.characters));
  double error_and_backspace_rate =
      (error_and_backspace_counter / static_cast<double>(verse_row.characters));

  if (completed_test ==
      true) { // Incrementing our overall and within-session test counters:
    test_number++;
    within_session_test_number++;
    // Note that the session number stays the same.

    Term::cout << "You typed the " << verse_row.characters
               << "-character verse \
in " << test_seconds
               << " seconds, which reflects a typing speed \
of " << wpm << " WPM. Your error rate was "
               << error_rate << " (not \
including backspaces) and "
               << error_and_backspace_rate << " (\
including backspaces)."
               << std::endl;

    /* The code below for looping through a dictionary is based on that
    provided by POW at https://stackoverflow.com/a/26282004/13097194 . */

    for (auto [word_map_key, word_map_value] : word_map) {
      // Term::cout << word_map_value.word << " "
      // << word_map_value.word_length << " " << word_map_value.wpm
      // << word_map_value.test_seconds << std::endl;

      /* Adding a new word result row to wrrv: (one row will be added
      for each word. */
      Word_Result_Row wrr;
      wrr.test_number = test_number;
      wrr.unix_test_start_time = unix_test_start_time_as_long;
      wrr.word = word_map_value.word;
      wrr.wpm = word_map_value.wpm;
      wrr.error_rate = word_map_value.error_rate;
      wrr.error_and_backspace_rate = word_map_value.error_and_backspace_rate;
      wrrv.push_back(wrr);
    }

    // // Now that we've completed our test, we can add the local
    // // keypress processing time to our main vector:
    // for (auto kptr: local_kptrv)
    // {
    // // Adding test_number attribute to kptr:
    // // (We could have done this while the player was completing
    // // the test, but we would have had to increment it by 1
    // // since the value hadn't yet been updated to reflect
    // // the completed test.)
    // kptr.test_number = test_number;
    // kptrv.push_back(kptr);
    // }

    /* Adding a new test result row to trrv: (only one row will
    be added for the entire test.) */
    Test_Result_Row trr;
    trr.test_number = test_number;
    trr.session_number = session_number;
    trr.within_session_test_number = within_session_test_number;
    trr.unix_test_start_time = unix_test_start_time_as_long;
    trr.local_test_start_time = local_start_time_string;
    trr.unix_test_end_time = unix_test_end_time_as_long;
    trr.local_test_end_time = local_end_time_string;
    trr.verse_id = verse_row.verse_id;
    trr.verse_code = verse_row.verse_code;
    trr.verse = verse_row.verse;
    trr.characters = verse_row.characters;
    trr.wpm = wpm;
    trr.test_seconds = test_seconds;
    trr.error_rate = error_rate;
    trr.error_and_backspace_rate = error_and_backspace_rate;
    trr.marathon_mode = marathon_mode;
    trr.player = player;
    trr.mode = mode;
    trr.tag_1 = tag_1;
    trr.tag_2 = tag_2;
    trr.tag_3 = tag_3;
    trr.notes = notes;
    trrv.push_back(trr);

    // Incrementing the 'tests' value (and, if needed,
    // updating the 'best_wpm' value) within our verse_row object:
    verse_row.tests += 1;
    if (wpm > verse_row.best_wpm) {
      verse_row.best_wpm = wpm;
    }
  } else {
    Term::cout << "Exiting test." << std::endl;
  }

  return completed_test;
}

Game_Config initialize_game_config(std::string mode) {
  // This function allows you to use the game_config.csv file in the
  // Files folder to initialize a player's Game_Config settings.

  // mode: the value to store within the 'mode' argument of the
  // configuration file.

  Game_Config gcf;

  std::string game_config_file_path = "../Files/game_config.csv";
  CSVReader game_config_reader(game_config_file_path);
  /* There should only be one row (not including the header row)
  within game_config.csv. If additional rows were somehow
  added, the final value for each row will be stored within gcf.
  I imagine that there's a way to update the following code to
  read only the first data row of the .csv file, but this approach
  (the same one used for earlier .csv import processes) will
  work for now. */

  for (auto &row : game_config_reader) {
    gcf.player = row["Player"].get<>();
    gcf.tag_1 = row["Tag_1"].get<>();
    gcf.tag_2 = row["Tag_2"].get<>();
    gcf.tag_3 = row["Tag_3"].get<>();
    gcf.notes = row["Notes"].get<>();
    gcf.mode = mode; // Can be SP for single player or
                     // MP for multiplayer.
  }

  Term::cout << "Initial configuration settings: Player: '" << gcf.player
             << "'\nMode: '" << gcf.mode << "'\nTag_1: '" << gcf.tag_1
             << "'\nTag_2: '" << gcf.tag_2 << "'\nTag_3: '" << gcf.tag_3
             << "'\nNotes: '" << gcf.notes << "'" << std::endl;
  return std::move(gcf);
}

void update_game_config(Game_Config &gcf)
/* This function allows the player to update game configuration
settings during the game. The output won't overwrite the defaults
stored within game_config.csv, but it will allow different
tags and player names to get stored within the test_results.csv
file. */
{
  std::string config_response = "";
  while (config_response != "e") {
    std::string new_config_setting = "";
    Term::cout << "Please enter your desired game \
configuration update. \
This update should start with a 'p' for a Player update; a '1' \
for a Tag_1 update; a '2' for a Tag_2 update; a '3' for a \
Tag_3 update; or an 'n' for a Notes update. This character should \
be followed by an underscore, \
then your desired new value for this configuration entry--which \
cannot contain any whitespace. (If you wish \
to make a given setting blank, enter only the initial code, e.g. \
'p' for Player or '2' for Tag_2.) \
Once you have finished making your desired updates, enter 'e' to \
return to the main gameplay menu.\nFor reference, here are your \
current configuration settings:\nPlayer: '"
               << gcf.player << "'\nTag_1: '" << gcf.tag_1 << "'\nTag_2: '"
               << gcf.tag_2 << "'\nTag_3: '" << gcf.tag_3 << "'\nNotes: '"
               << gcf.notes << "'" << std::endl;

    config_response = cooked_input_within_raw_mode();
    // Checking the value of config_response for debugging purposes:
    Term::cout << "\nValue of config_response:\n"
               << config_response << std::endl;

    // Checking to see whether the user wishes to exit:
    if (config_response == "e") {
      Term::cout << "Finished updating configuration options." << std::endl;
      continue;
    }

    // Using the first character within config_response to determine
    // which configuration setting should be updated with this new value:
    char config_setting_code = config_response[0];

    // Making sure that a valid code was entered:
    std::vector<char> valid_codes{'p', '1', '2', '3', 'n'};

    if (std::find(valid_codes.begin(), valid_codes.end(),
                  config_setting_code) == valid_codes.end())
    // This code was based on an example found at
    // https://en.cppreference.com/w/cpp/algorithm/find.html .
    // The 'true' condition here means that the code was *not* found
    // within valid_codes.
    {
      Term::cout << "The configuration setting code \
should be 'p', '1', \
'2', '3', or 'n.' Please try again or enter 'e' to exit."
                 << std::endl;
      continue;
    }

    // The following map, which stores each code's corresponding full
    // setting name, will help make an upcoming dialog
    // easier to interpret.
    std::map<char, std::string> code_to_config_map{{'p', "Player"},
                                                   {'1', "Tag_1"},
                                                   {'2', "Tag_2"},
                                                   {'3', "Tag_3"},
                                                   {'n', "Notes"}};

    std::string new_config_value = "";

    // Checking whether a non-blank entry for the code was provided:
    if (config_response.size() >= 3)

    { // Extracting the new value for the configuration setting:
      // (this value starts at position 2 within the user's response and
      // continues on until the end of the string.)
      new_config_value = config_response.substr(2, config_response.size() - 2);
    }

    // If this if statement returns false, no value was provided, so
    // the default value for new_config_value will be retained.

    Term::cout << "Your requested new "
               << code_to_config_map[config_setting_code] << " setting is '"
               << new_config_value << "'. To confirm this change, \
enter 'y.' To try again, enter 'n.'"
               << std::endl;

    std::string config_change_confirmation = "";

    config_change_confirmation = get_single_keypress({"y", "n"});

    if (config_change_confirmation != "y") // If anything other
    // than 'y' is entered by the user, the dialog will
    // restart, and no changes will be made.
    {
      Term::cout << "This change has been canceled." << std::endl;
      continue;
    }

    if (config_setting_code == 'p') {
      gcf.player = new_config_value;
    }

    else if (config_setting_code == '1') {
      gcf.tag_1 = new_config_value;
    }

    else if (config_setting_code == '2') {
      gcf.tag_2 = new_config_value;
    }

    else if (config_setting_code == '3') {
      gcf.tag_3 = new_config_value;
    }

    else if (config_setting_code == 'n') {
      gcf.notes = new_config_value;
    }
    Term::cout << "Updated configuration setting." << std::endl;
  }
}

std::vector<Verse_Row> import_verses(const std::string &verses_file_path) {

  // This function creates a vector that can store all of the Bible
  // verses found within CPDB_for_TTTB.csv. Storing this code within
  // its own function allows it to be utilized within both the
  // single-player and multiplayer gameplay modes.

  // Note: I had considered reading data into a vector of
  // unique pointers to Verse_Row objects; however, based on
  // some online research, I don't think this approach would
  // necessarily speed up my program--and, in fact, all of the
  // dereferencing operations could end up slowing it down. Therefore,
  // I instead decided to have vrv store full copies of this data;
  // however, my typing test function accesses values stored in this
  // vector by reference, which should reduce the amount of
  // copying involved.
  //

  auto vrv_import_start_time = std::chrono::high_resolution_clock::now();

  std::vector<Verse_Row> vrv; // vrv is short for 'Verse Row vector.'

  /* Reading in data:
  This code was based largely on
  https://github.com/vincentlaucsb/csv-parser?tab=
  readme-ov-file#indexing-by-column-names
  and on
  page 22 of A Tour of C++, 2nd Edition by Bjarne Stroustrup. */
  CSVReader reader(verses_file_path);
  for (auto &row : reader) {
    Verse_Row vr;
    vr.verse_id = row["Verse_ID"].get<int>();
    vr.ot_nt = row["OT_NT"].get<std::string>();
    vr.book = row["Book"].get<std::string>();
    vr.book_num = row["Book_Num"].get<int>();
    vr.chapter_num = row["Chapter_Num"].get<std::string>();
    vr.verse_num = row["Verse_Num"].get<int>();
    vr.verse_code = row["Verse_Code"].get<std::string>();
    vr.verse = row["Verse"].get<std::string>();
    vr.characters = row["Characters"].get<int>();
    vr.tests = row["Tests"].get<int>();
    vr.best_wpm = row["Best_WPM"].get<double>();
    vrv.push_back(vr);
  }

  auto vrv_import_end_time = std::chrono::high_resolution_clock::now();

  auto vrv_import_seconds =
      std::chrono::duration<double>(vrv_import_end_time - vrv_import_start_time)
          .count();

  Term::cout << "Imported " << vrv.size() << " Bible verses in "
             << vrv_import_seconds << " seconds." << std::endl;
  return (std::move(vrv));
}

std::vector<Test_Result_Row>
import_test_results(std::string &test_results_filename)
// This function imports an existing set of test results. It is
// currently being applied within the import_mp_results() function,
// but it could also be applied to help add autosave data to
// an existing set of test results.
{
  std::vector<Test_Result_Row> trrv;
  CSVReader reader(test_results_filename);
  for (auto &row : reader) {
    Test_Result_Row trr;

    trr.test_number = row["Test_Number"].get<long>();
    trr.session_number = row["Session_Number"].get<long>();
    trr.within_session_test_number =
        row["Within_Session_Test_Number"].get<int>();
    trr.unix_test_start_time = row["Unix_Test_Start_Time"].get<long>();
    trr.local_test_start_time = row["Local_Test_Start_Time"].get<>();
    trr.unix_test_end_time = row["Unix_Test_End_Time"].get<long>();
    trr.local_test_end_time = row["Local_Test_End_Time"].get<>();
    trr.verse_id = row["Verse_ID"].get<int>();
    trr.verse_code = row["Verse_Code"].get<std::string>();
    trr.verse = row["Verse"].get<std::string>();
    trr.characters = row["Characters"].get<int>();
    trr.wpm = row["WPM"].get<double>();
    trr.test_seconds = row["Test_Seconds"].get<double>();
    trr.error_rate = row["Error_Rate"].get<double>();
    trr.error_and_backspace_rate =
        row["Error_and_Backspace_Rate"].get<double>();
    trr.marathon_mode = row["Marathon_Mode"].get<int>();
    trr.player = row["Player"].get<std::string>();
    trr.mode = row["Mode"].get<std::string>();
    trr.tag_1 = row["Tag_1"].get<std::string>();
    trr.tag_2 = row["Tag_2"].get<std::string>();
    trr.tag_3 = row["Tag_3"].get<std::string>();
    trr.notes = row["Notes"].get<std::string>();
    trrv.push_back(trr);
  }
  return std::move(trrv);
}

std::vector<Word_Result_Row>
import_word_results(std::string &word_results_filename)
// This function imports an existing set of word-level results.
{
  std::vector<Word_Result_Row> wrrv;
  CSVReader reader(word_results_filename);
  for (auto &row : reader) {
    Word_Result_Row wrr;

    wrr.test_number = row["Test_Number"].get<long>();
    wrr.unix_test_start_time = row["Unix_Test_Start_Time"].get<long>();
    wrr.word = row["Word"].get<std::string>();
    wrr.wpm = row["WPM"].get<double>();
    wrr.error_rate = row["Error_Rate"].get<double>();
    wrr.error_and_backspace_rate =
        row["Error_and_Backspace_Rate"].get<double>();
    wrrv.push_back(wrr);
  }
  return std::move(wrrv);
}

void export_test_results(const std::vector<Test_Result_Row> &trrv,
                         const std::string &test_results_file_path,
                         bool include_header_row = false,
                         bool autosave_mode = false) {
  // This function exports a set of test results to a specified file
  // path.
  // If include_header_row is set to true, a header row will be
  // added to the beginning of the .csv file. This will only be
  // necessary for multiplayer games; in all other cases, a header
  // row will already be present, or its addition could cause
  // issues later on (e.g. when trying to add autosave data
  // to a .csv file.)
  // If autosave_mode is set to true, the original contents
  // of test_results_file_path will get cleared out and replaced
  // with the latest contents. This is desirable for
  // autosave files (as it prevents duplicate copies of the same
  // results from getting written). If set to false,
  // the function will instead *append* the latest set of results
  // to any pre-existing results within test_results_file_path,
  // which should save time relative to first loading all
  // pre-existing results into the game.

  auto trrv_export_start_time = std::chrono::high_resolution_clock::now();

  // Determining whether to clear the file to which data will
  // be written or append new rows to existing ones:
  // For more details on std::ios::app (which instructs the program
  // to append new lines to the end of this existing file),
  // see https://en.cppreference.com/w/cpp/io/basic_ofstream.html
  // and https://stackoverflow.com/a/76151007/13097194 .
  // For the use of std::ios_base::openmode,
  // see https://en.cppreference.com/w/cpp/io/ios_base/openmode .
  // This StackOverflow post by mark sabido indicates that
  // we'll need to specify trunc in order to replace a longer
  // set of autosave data (i.e. from a previous game) with
  // a shorter set:
  // https://stackoverflow.com/questions/43766410/
  // overwrite-an-existing-text-file-c#comment126848651_43766778

  std::ios_base::openmode write_mode = std::ios::app;

  if (autosave_mode == true) {
    write_mode = std::ios::trunc;
  }

  std::ofstream test_results_ofstream{test_results_file_path, write_mode};

  auto test_results_writer = make_csv_writer(test_results_ofstream);

  if (include_header_row == true) {
    std::vector<std::string> header_row = {"Test_Number",
                                           "Session_Number",
                                           "Within_Session_Test_Number",
                                           "Unix_Test_Start_Time",
                                           "Local_Test_Start_Time",
                                           "Unix_Test_End_Time",
                                           "Local_Test_End_Time",
                                           "Verse_ID",
                                           "Verse_Code",
                                           "Verse",
                                           "Characters",
                                           "WPM",
                                           "Test_Seconds",
                                           "Error_Rate",
                                           "Error_and_Backspace_Rate",
                                           "Marathon_Mode",
                                           "Player",
                                           "Mode",
                                           "Tag_1",
                                           "Tag_2",
                                           "Tag_3",
                                           "Notes"};

    // Writing this header to the .csv file:
    test_results_writer << header_row;
  }

  for (int i = 0; i < trrv.size(); ++i) {
    std::vector<std::string> cols_as_strings = {
        std::to_string(trrv[i].test_number),
        std::to_string(trrv[i].session_number),
        std::to_string(trrv[i].within_session_test_number),
        std::to_string(trrv[i].unix_test_start_time),
        trrv[i].local_test_start_time,
        std::to_string(trrv[i].unix_test_end_time),
        trrv[i].local_test_end_time,
        std::to_string(trrv[i].verse_id),
        trrv[i].verse_code,
        trrv[i].verse,
        std::to_string(trrv[i].characters),
        std::to_string(trrv[i].wpm),
        std::to_string(trrv[i].test_seconds),
        std::to_string(trrv[i].error_rate),
        std::to_string(trrv[i].error_and_backspace_rate),
        std::to_string(trrv[i].marathon_mode),
        trrv[i].player,
        trrv[i].mode,
        trrv[i].tag_1,
        trrv[i].tag_2,
        trrv[i].tag_3,
        trrv[i].notes,
    };
    test_results_writer << cols_as_strings;
  };

  auto trrv_export_end_time = std::chrono::high_resolution_clock::now();
  auto trrv_export_seconds = std::chrono::duration<double>(
                                 trrv_export_end_time - trrv_export_start_time)
                                 .count();
  Term::cout << "Exported " << trrv.size() << " test result(s) in "
             << trrv_export_seconds << " seconds." << std::endl;
}

void export_word_results(const std::vector<Word_Result_Row> &wrrv,
                         const std::string &word_results_file_path,
                         bool include_header_row = false,
                         bool autosave_mode = false)
// This function is similar to export_test_results except that it
// saves word-level rather than test-level results.
{

  auto wrrv_export_start_time = std::chrono::high_resolution_clock::now();

  std::ios_base::openmode write_mode = std::ios::app;

  if (autosave_mode == true) {
    write_mode = std::ios::trunc;
  }

  std::ofstream word_results_ofstream{word_results_file_path, write_mode};

  auto word_results_writer = make_csv_writer(word_results_ofstream);

  if (include_header_row == true) {
    std::vector<std::string> header_row = {
        "Test_Number", "Unix_Test_Start_Time", "Word",
        "WPM",         "Error_Rate",           "Error_and_Backspace_Rate"};

    // Writing this header to the .csv file:
    word_results_writer << header_row;
  }

  for (int i = 0; i < wrrv.size(); ++i) {
    std::vector<std::string> cols_as_strings = {
        std::to_string(wrrv[i].test_number),
        std::to_string(wrrv[i].unix_test_start_time),
        wrrv[i].word,
        std::to_string(wrrv[i].wpm),
        std::to_string(wrrv[i].error_rate),
        std::to_string(wrrv[i].error_and_backspace_rate)};
    word_results_writer << cols_as_strings;
  };

  auto wrrv_export_end_time = std::chrono::high_resolution_clock::now();
  auto wrrv_export_seconds = std::chrono::duration<double>(
                                 wrrv_export_end_time - wrrv_export_start_time)
                                 .count();
  Term::cout << "Exported " << wrrv.size() << " word results in "
             << wrrv_export_seconds << " seconds." << std::endl;
}

// void export_keypress_processing_times(
//     const std::vector<Keypress_Processing_Time_Row> &kptrv,
//     const std::string &keypress_processing_time_file_path,
//     bool include_header_row = false,
//     bool autosave_mode = false)
// // This function is similar to export_test_results except that it
// // saves keypress_processing_time-level rather than test-level results.
// {

//     auto kptrv_export_start_time = std::chrono::
//         high_resolution_clock::now();

//     std::ios_base::openmode write_mode = std::ios::app;

//     if (autosave_mode == true)
//         {write_mode = std::ios::trunc;}

//     std::ofstream keypress_processing_time_results_ofstream{
//         keypress_processing_time_file_path, write_mode};

//     auto keypress_processing_time_results_writer = make_csv_writer(
//         keypress_processing_time_results_ofstream);

// if (include_header_row == true)
//     {std::vector<std::string> header_row = {
//         "Test_Number",
//         "Keypress_Number",
//         "Processing_Time"};

//     // Writing this header to the .csv file:
//     keypress_processing_time_results_writer << header_row;
//     }

//     for (int i = 0; i < kptrv.size(); ++i)
//     {
//         std::vector<std::string> cols_as_strings = {
//             std::to_string(kptrv[i].test_number),
//             std::to_string(kptrv[i].keypress_number),
//             std::to_string(kptrv[i].processing_time_in_microseconds)};
//         keypress_processing_time_results_writer << cols_as_strings;
//     };

//     auto kptrv_export_end_time = std::chrono::
//         high_resolution_clock::now();
//     auto kptrv_export_seconds = std::chrono::duration<double>(
//     kptrv_export_end_time - kptrv_export_start_time).count();
//     Term::cout << "Exported " << kptrv.size() << " keypress \
// processing durations in " << kptrv_export_seconds << " seconds."
// << std::endl;
// }

void export_verses(const std::vector<Verse_Row> &vrv,
                   const std::string &verses_file_path)
/* This function saves the updated copy of our table of Bible
verses (with new completed-test and best-WPM data) to a .csv file.
Because these exports will always include headers, and because
verses will never need to be appended to an existing set of
verses, this function does not need the include_header_row
and autosave_mode parameters found within export_test_results()
and export_word_results().

This section was based on the documentation found at
https://github.com/vincentlaucsb/csv-parser?
tab=readme-ov-file#writing-csv-files
and
https://vincela.com/csv/classcsv_1_1DelimWriter.html .*/
{
  auto vrv_export_start_time = std::chrono::high_resolution_clock::now();

  std::ofstream verse_ofstream{verses_file_path};
  auto verses_writer = make_csv_writer(verse_ofstream);

  // Writing header row to .csv file:
  // It's crucial that the order of these rows matches the order
  // of the corresponding field entries within cols_as_strings.
  std::vector<std::string> header_row = {
      "Verse_ID",    "OT_NT",     "Book",       "Book_Num",
      "Chapter_Num", "Verse_Num", "Verse_Code", "Verse",
      "Characters",  "Tests",     "Best_WPM"};

  // Writing this header to the .csv file:
  verses_writer << header_row;

  // Converting the fields within each row into a vector
  // of strings:
  // See
  // https://stackoverflow.com/a/23855901/13097194
  // (which actually recommends a potentially better solution
  // outside of the standard library) for to_string().
  for (int i = 0; i < vrv.size(); ++i) {
    std::vector<std::string> cols_as_strings = {
        std::to_string(vrv[i].verse_id),
        vrv[i].ot_nt,
        vrv[i].book,
        std::to_string(vrv[i].book_num),
        vrv[i].chapter_num,
        std::to_string(vrv[i].verse_num),
        vrv[i].verse_code,
        vrv[i].verse,
        std::to_string(vrv[i].characters),
        std::to_string(vrv[i].tests),
        std::to_string(vrv[i].best_wpm)};
    verses_writer << cols_as_strings;
  };

  auto vrv_export_end_time = std::chrono::high_resolution_clock::now();
  auto vrv_export_seconds =
      std::chrono::duration<double>(vrv_export_end_time - vrv_export_start_time)
          .count();
  Term::cout << "Exported " << vrv.size() << " Bible verses in "
             << vrv_export_seconds << " seconds." << std::endl;
}

std::map<std::string, long> count_sp_test_results()
// This function locates the highest existing test and //
// session numbers within
// the test_results.csv file, then returns these numbers.
// (Note: a previous version of this function simply counted the
// number of test results; however, if rows prior to the final
// result within that file ever got deleted, that approach could
// lead to duplicate test result IDs--which could cause all sorts
// of issues. The current approach helps prevent duplicate test
// results from appearing.
{
  long test_number = 0; // This value will be updated as needed
  // to match the highest test number completed.
  long session_number = 0;
  long tests_completed = 0; // This number may end up being
  // less than test_number if rows prior to the last row
  // within test_results.csv had gotten deleted at some point.
  CSVReader reader("../Files/test_results.csv");
  // Iterating through our test results .csv file in order to
  // determine how many tests have already been completed and
  // update our test_number accordingly:
  // (test_number, along with within_session_test_number,
  // will get incremented following completed tests within
  // run_test).
  Term::cout << "Now looping through test results." << std::endl;
  for (auto &row : reader) {
    tests_completed++;
    long row_test_number = row["Test_Number"].get<long>();
    long row_session_number = row["Session_Number"].get<long>();
    if (row_test_number >= test_number) {
      test_number = row_test_number;
    }
    if (row_session_number >= session_number) {
      session_number = row_session_number;
    }
  }
  Term::cout << tests_completed << " tests and " << session_number
             << " sessions have been \
completed so far."
             << std::endl;
  // For debugging:
  Term::cout << "The highest test number so far is " << test_number << "."
             << std::endl;

  return std::map<std::string, long>{{"test_number", test_number},
                                     {"session_number", session_number}};
}

void run_single_player_game(std::string py_complement_name) {
  // Term::cout << "Now calling count_sp_test_results." << std::endl;
  std::map<std::string, long> test_and_session_numbers =
      count_sp_test_results();
  long test_number = test_and_session_numbers["test_number"];
  // Adding 1 to the session number so that it will be distinct from
  // the previous session's number:
  // (test_number values will get incremented elsewhere within the
  // game.)
  long session_number = test_and_session_numbers["session_number"] + 1;
  int within_session_test_number = 0;

  // Importing Bible verses:
  // Note: it will be important to run this code at the start of
  // each game. If we only imported it at the very start of each
  // gameplay session, and the player ended up doing multiple
  // single-player games, it's possible that only the final set
  // of results would ultimately get saved to the Bible verse file.
  // Re-importing that file here allows any updates created within
  // previous single-player gameplay sessions to get incorporated
  // into the new session.

  std::vector<Verse_Row> vrv = import_verses(verses_file_path);

  // Importing test result data:
  // I was also thinking about having trrv store unique pointers
  // to test result rows, but I ultimately decided to instead
  // have my typing test function access this data by reference,
  // as I felt that that would lead to better performance overall.

  auto trrv_import_start_time = std::chrono::high_resolution_clock::now();

  std::vector<Test_Result_Row> trrv; // trrv is short for
  // 'Test result row vector.'

  // std::vector<Keypress_Processing_Time_Row> kptrv; // kptrv is
  //  short for 'Keypress processing time row vector.'

  std::string test_results_file_path = "../Files/test_results.csv";
  std::string autosaved_test_results_file_path = "../Files/\
autosaved_test_results.csv";

  std::vector<Word_Result_Row> wrrv; // wrrv is short for
  // 'Word result row vector.'

  std::string word_results_file_path = "../Files/word_results.csv";
  // std::string keypress_processing_time_file_path = (
  //     "../Files/keypress_processing_time.csv");
  std::string autosaved_word_results_file_path = "../Files/\
autosaved_word_results.csv";

  // Counting the number of verses and characters the player
  // has already typed:

  int earliest_untyped_verse_index = 40000; // This number will be
  // updated within the following for loop.
  int typed_characters = 0;
  double top_within_session_wpm = 0.0;
  double total_characters = 0; // Setting this as a double so that
  // typed characters / all characters quotients will
  // themselves be doubles.
  int untyped_verses = 0;
  int typed_verses = 0;
  bool all_verses_typed = true; // This value will also be used
  // within our main gameplay loop to determine whether or not
  // the user has finished typing all verses at least once.

  long characters_typed_during_current_session = 0;

  for (int i = 0; i < vrv.size(); ++i) {
    total_characters += vrv[i].characters;
    if (vrv[i].tests == 0) {
      if (i < earliest_untyped_verse_index)
      // In this case, our earliest untyped verse index value
      // will be updated to match this smaller value. (We
      // deliberately initialized that variable to a number
      // higher than our verse count in order to ensure that this
      // update process would work correctly.
      {
        earliest_untyped_verse_index = i;
      }
      all_verses_typed = false;
      untyped_verses++;
    } else {
      typed_verses++;
      typed_characters += vrv[i].characters;
    }
  }

  if (untyped_verses != 0) {
    Term::cout << typed_verses << " unique Bible verses have \
been typed so far, leaving "
               << untyped_verses << " untyped. The earliest untyped \
verse has the ID "
               << earliest_untyped_verse_index + 1 << "." << std::endl;

    Term::cout << typed_characters << " unique characters from the \
Bible have been typed, which represents "
               << 100 * (typed_characters / total_characters)
               << " % of the Bible." << std::endl;
  }

  else {
    Term::cout << "All verses have been typed! Congratulations on \
this rare accomplishment!"
               << std::endl;
  }

  int previously_typed_verse_index = -1; // I chose to initialize
  // this integer as -1 so that, when the user selects
  // sequential marathon mode at the very start of the program,
  // TTTB will select the verse with index 0 (e.g. the first verse
  // in the Bible) as their first verse to type.

  // Reading in default configuration settings specified by the user
  // within config.csv:
  // (These settings can be updated within the game as needed.)

  Game_Config gcf = initialize_game_config("SP");

  std::string user_response = "";
  bool marathon_mode = false;
  bool show_results_before_test_starts = false; // If set to true,
  // results and progress updates will be shown in run_test() before
  // the test begins; otherwise, they will be shown within the
  // following loop. (Elements of the following loop will be skipped
  // for certain game modes, such as N; thus, I needed to add in a
  // variable that would instruct the game to instead show these results
  // within run_test so that the user could keep better track of his/
  // her progress within such modes.)

  std::vector<double> session_wpm_results = {}; // Stores all WPM values
  // from tests completed during this session (which will allow us
  // to share WPM-related stats before non-marathon races).

  // Main single-player gameplay loop:
  while (user_response != "e") {
    std::string progress_message = ""; // This message, which
                                       // this loop will update, will
    // be displayed either within this gameplay loop or within
    // run_test (depending on what gameplay setting the user
    // has chosen).

    int verse_index_to_type = -1; // We'll check for this value when
    // determining whether to run a typing test.

    // Calculating various within-session statistics that will
    // be shown to the user shortly:

    std::string post_entry_message = ""; // This message will be
    // replaced with a brief stats update when marathon mode is
    // active; otherwise, it will be kept as an empty string.

    // Checking whether our top-within-session WPM value needs
    // to be updated:
    if (within_session_test_number >= 1) {
      if (trrv.back().wpm > top_within_session_wpm) {
        top_within_session_wpm = trrv.back().wpm;
      }
    }
    // Calculating the player's average WPM across (1) all races
    // completed this session and (2) the last 10 races:

    double last_10_wpm_sum = 0;
    double session_wpm_sum = 0;
    double session_wpm_mean = 0;
    double last_10_wpm_mean = 0;

    if (within_session_test_number >= 2) {
      for (int i = 0; i < within_session_test_number; ++i) {
        session_wpm_sum += session_wpm_results[i];
        // Checking whether at least 10 races have been completed
        // thus far
        if (within_session_test_number >= 10) {
          // Determining whether i is currently accessing one of the
          // last 10 races completed by the user
          if (i >= (within_session_test_number - 10)) {
            last_10_wpm_sum += session_wpm_results[i];
          }
        }
      }
      session_wpm_mean = (session_wpm_sum / within_session_test_number);
      last_10_wpm_mean = last_10_wpm_sum / 10;
    }

    if (marathon_mode & (within_session_test_number >= 1)) {
      // Creating a short stats update that can be displayed after a
      // player's response when marathon mode is active:
      // (That way, the player will still be able to keep track of his/her
      // progress, as other update messages will be skipped within
      // this mode.)
      // (More information will be added to this message
      // as more tests get completed.)
      {
        post_entry_message =
            "\n\nStats for last verse (" + trrv.back().verse_code +
            "): WPM: " + std::to_string(trrv.back().wpm) + "; E&B rate: " +
            std::to_string(trrv.back().error_and_backspace_rate) + "\n";
      }

      if (within_session_test_number >= 2) {
        post_entry_message +=
            "Session stats: " +
            std::to_string(characters_typed_during_current_session) +
            " characters; " + std::to_string(within_session_test_number) +
            " verses; " + std::to_string(session_wpm_mean) + "/" +
            std::to_string(top_within_session_wpm) + " mean/max WPM";
      }

      if (within_session_test_number >= 10) {
        post_entry_message += ";\n" + std::to_string(last_10_wpm_mean) +
                              " mean WPM over the last 10 tests";
      }
    }

    else // Specifying what messages to display if marathon
    // mode is *not* active--and prompting the user to select a game mode
    {

      if ((show_results_before_test_starts == true) &&
          (within_session_test_number >= 1)) {
        // In this case,
        // the user won't be able to view the normal
        // post-race results message, so we'll add it here
        // instead. (This message also shows the verse code
        // in order to clarify, in the result of a canceled
        // test, the verse to which this result belongs.)

        // If show_results_before_test_starts is instead set to False,
        // a similar message will be displayed after the test is
        // finished, so we won't append this text to
        // progress_message (as doing so would create redundant
        // output).

        progress_message +=
            "You typed " + trrv.back().verse_code + ", a(n) " +
            std::to_string(trrv.back().characters) + "-character verse, \
in " + std::to_string(trrv.back().test_seconds) +
            " seconds, \
which reflects a typing speed of " +
            std::to_string(trrv.back().wpm) + " WPM. Your error rate \
was " + std::to_string(trrv.back().error_rate) +
            " (not \
including backspaces) and " +
            std::to_string(trrv.back().error_and_backspace_rate) +
            " (including backspaces).\n";
      }

      if (within_session_test_number >= 2) {
        progress_message +=
            "You have typed " +
            std::to_string(characters_typed_during_current_session) +
            " characters \
so far this session. Your mean and top WPM over the " +
            std::to_string(within_session_test_number) +
            " tests you have completed this session are " +
            std::to_string(session_wpm_mean) + " and " +
            std::to_string(top_within_session_wpm) + ", respectively.\n";
      };

      if (within_session_test_number >= 10) {
        progress_message += "Your mean WPM over your last 10 \
races is " + std::to_string(last_10_wpm_mean) +
                            ".\n";
      }
      // The following code displays a different set of colors
      // after each test; the patterns will repeat every
      // 256 tests. (There's no real purpose for doing so;
      // I just thought it would be nice to add a bit more
      // color to the terminal display!)
      progress_message +=
          background_color_prefix +
          background_color_codes[(within_session_test_number / 16) % 16] +
          background_color_suffix + background_color_prefix +
          background_color_codes[within_session_test_number % 16] +
          background_color_suffix;

      if (show_results_before_test_starts == false) // In this case,
      // we'll display the progress message here rather than
      // within the next run_test() call.
      {
        Term::cout << progress_message << std::endl;

        Term::cout << "Choose your desired game mode by typing \
'n', 'c', 'i', 'r', 'N', 'C', 'I', 'm', 's', 'u', or 'e'. \
Press 'h' for descriptions of these options."
                   << std::endl;

        user_response = get_single_keypress(
            {"n", "c", "i", "r", "m", "s", "u", "e", "N", "C", "I", "h"});
      }
    }

    // Instructing the game to show progress messages within
    // run_test rather than this loop if N or C was requested:
    if ((user_response == "N") || (user_response == "C") ||
        (user_response == "I")) {
      show_results_before_test_starts = true;
    } else {
      show_results_before_test_starts = false;
    }

    if (user_response == "h") // This choice will display more
    // information about each game mode; the user will then
    // be taken back to the start of the main gameplay loop.

    {
      Term::cout << "Description of game modes:\n\
'n'\tType the next untyped verse, then return to the gameplay menu.\n\
'c'\tType the next verse, then return to the gameplay menu.\n\
'i'\tType a specific verse ID, then return to the gameplay menu.\n\
'r'\tRepeat the verse you just typed, then return to the \
gameplay menu.\n\
'N'\tAutomatically get directed to the next untyped verse.\n\
'C'\tAutomatically get directed to the next verse.\n\
'I'\tSelect a verse from which to begin, then \
automatically get directed to the verses that follow it.\n\
'm'\tEnter 'untyped marathon mode,' in which you will immediately \
start a test for the next untyped verse after finishing the current \
test.\n\
's'\tEnter 'sequential marathon mode,' in which you will immediately \
start (1) the verse following the one you just typed or \
(2) (when starting this mode) a verse of your choice.\n\
'u'\tUpdate game configuration settings.\n\
'e'\tExit this session and save your progress.\n\
Also remember that you can exit out of any test, even in marathon \
mode, by pressing Ctrl+C."
                 << std::endl;
    }

    if (user_response == "u") // This option will allow the user
    // to update game configuration settings for the current session.
    {
      update_game_config(gcf);
    }

    else if ((user_response == "n") || (user_response == "m") ||
             (user_response == "N"))

      if (all_verses_typed == true) {
        Term::cout << all_verses_typed_message << std::endl;
        marathon_mode = false;
        // Since verse_index_to_type is still at -1, the user
        // will now be returned to the main gameplay prompt.
        // (It may not actually be necessary to set
        // marathon_mode to false here, but it likely
        // won't hurt to include this code just in case.)
        show_results_before_test_starts = false;
        // Resetting this boolean to false so that the user
        // can pick a new gameplay code
      } else {
        // Checking for the next verse that hasn't been typed:
        // (It's best to perform this check anew whenever the user enters
        // this response to account for verses that were specified manually
        // via the 'i' argument, then completed.)

        // Note: this loop makes use of a previously defined variable
        // that will get updated whenever this loop runs. (That way,
        // we won't waste time checking earlier verses that we've already
        // checked.)

        for (; earliest_untyped_verse_index < vrv.size();
             ++earliest_untyped_verse_index) {
          if (vrv[earliest_untyped_verse_index].tests == 0) {
            verse_index_to_type = earliest_untyped_verse_index;
            if (user_response == "m") {
              marathon_mode = true;
            }
            break;
          }
        }

        if (verse_index_to_type == -1) // In this case,
        // the user has typed all verses at least once.
        {
          all_verses_typed = true;
          Term::cout << all_verses_typed_message << std::endl;
        }
      }

    else if ((user_response == "i") || (user_response == "I")) {
      verse_index_to_type = select_verse_id(vrv);
      if (user_response == "I") // In this case, now that
      // the desired verse to type has been selected,
      // user_response will be changed to 'C' so that
      // the user will continue to be presented consecutive
      // verses (either typed or untyped) that follow after
      // this first verse.)
      {
        user_response = "C";
      }
    }

    else if (user_response == "r") {
      if (within_session_test_number == 0) {
        Term::cout << "No verses have been typed yet, so this \
option isn't valid.\n"
                   << std::endl;
      } else {
        verse_index_to_type = previously_typed_verse_index;
      }
    }

    else if (user_response == "e") {
      Term::cout << "Exiting game and saving progress." << std::endl;
    }

    else if ((user_response == "s") || (user_response == "c") ||
             (user_response == "C")) {
      if (previously_typed_verse_index == (vrv.size() - 1))
      // In this case, the previously-typed verse
      // was the last verse in the Bible; therefore, this option
      // won't be valid.
      {
        Term::cout << "You just typed the last verse in \
the Bible. therefore, you'll need to select another option, \
such as 'i', which will allow you to type a specific verse."
                   << std::endl;
        marathon_mode = false; // Exiting marathon mode in order to prevent
        // an infinite dialog loop from occurring
        show_results_before_test_starts = false;
      } else {
        verse_index_to_type = previously_typed_verse_index + 1;
        if (user_response == "s") {
          if (marathon_mode == false) // In this case, the player
          // has just entered this mode; therefore, we will allow
          // him/her to select a starting verse.
          {
            Term::cout << "Please select a verse ID \
from which to start this mode."
                       << std::endl;
            verse_index_to_type = select_verse_id(vrv);
            if (verse_index_to_type != -1) // If the user didn't select
            // a valid verse, we won't want to set marathon mode to true.
            {
              marathon_mode = true;
            }
          }
        }
      }
    }

    if (verse_index_to_type != -1) // In this case, the user has
    // indicated that he/she wishes to type a verse--so we'll go ahead
    // and initiate a typing test for the verse in question.
    {
      // Running the typing test:
      // Note: All arguments will be passed as references in order to
      // (hopefully) reduce runtime and/or allow certain items,
      // including trrv and wrrv, to get updated directly within
      // the function.
      std::string within_test_update_message = "";
      if (show_results_before_test_starts == true) {
        within_test_update_message = progress_message;
      }

      bool completed_test = run_test(
          vrv[verse_index_to_type], trrv, wrrv, marathon_mode, gcf.player,
          gcf.mode, gcf.tag_1, gcf.tag_2, gcf.tag_3, gcf.notes, test_number,
          session_number, within_session_test_number, true,
          // kptrv,
          within_test_update_message, post_entry_message);
      // The following code will exit a user out of either marathon
      // mode if he/she did not complete the most recent test.
      // It will also cause the pre-test menu to reappear
      // if the user had disabled it via the 'N', 'C', or 'I'
      // game modes.
      if (completed_test == false) {
        marathon_mode = false;
        show_results_before_test_starts = false;
      }

      if (completed_test == true)
      // Updating previously_typed_verse_index so that it will be
      // ready for use within options 's' and 'c':
      {
        previously_typed_verse_index = verse_index_to_type;
        // Adding the most recent WPM within trrv (i.e. the WPM of the
        // test that was just completed) to our vector of session-level
        // WPM results:

        session_wpm_results.push_back(trrv.back().wpm);
        characters_typed_during_current_session += (trrv.back().characters);
      }

      // Every 10 races, the player's updated Bible verse file,
      // test-level results, and word-level results will be
      // saved to autosave files.
      if ((within_session_test_number > 0) &&
          (within_session_test_number % 10 == 0)) {
        Term::cout << "Performing autosave." << std::endl;
        export_verses(vrv, autosaved_verses_file_path);
        export_test_results(trrv, autosaved_test_results_file_path, false,
                            true);
        export_word_results(wrrv, autosaved_word_results_file_path, false,
                            true);
        Term::cout << "Autosave complete." << std::endl;
      }
    }
  }

  // Exporting Bible verses, test results, and word results:
  export_verses(vrv, verses_file_path);

  export_test_results(trrv, test_results_file_path, false, false);

  export_word_results(wrrv, word_results_file_path, false, false);

  // export_keypress_processing_times(kptrv,
  // keypress_processing_time_file_path,
  // false, false);

  // Calculating single-player stats:

  // Attempting to call Python file for converting single-player
  // results into visualizations:
  // Note: You'll likely need to check whether the user is running
  // Windows (and possibly Mac), and if so, run a slightly different
  // command.
  // The following code was based on the examples shown at
  // https://www.geeksforgeeks.org/cpp/system-call-in-c/ .

  Term::cout << "\nPress 'y' to call a Python script that updates \
single-player stats; type 'n' to skip this process. (This may \
require some setup on your part; \
see Readme for more details. You can also run that script \
separately if you prefer."
             << std::endl;

  std::string sp_stats_update_response = "";

  sp_stats_update_response = get_single_keypress({"y", "n"});

  if (sp_stats_update_response == "y") {

    // In order to determine how to write the upcoming system call,
    // we'll need to see whether our code is running on
    // Windows.

    Term::cout << "Calling Python script. (This can take \
a little while.)"
               << std::endl;

    std::string system_call = py_complement_name + " spv spare_arg";

    try {
      // Only one argument (spv) needs to be passed to
      // tttb_py_complement for single-player stats; however,
      // since this function still expects two arguments, I'll
      // pass a second argument to it anyway. (There's probably
      // a way to eliminate the need for this second argument,
      // but it's past midnight and this approach will work for now. :)

      // Converting this string to a C-style string, then passing
      // it to system():
      std::system(system_call.c_str());
    } catch (...) {
      Term::cout << "Unable to run system command." << std::endl;
    }
  }

  else {
    Term::cout << "Skipping single-player stats updates." << std::endl;
  }

  Term::cout << "Quitting single-player gameplay session." << std::endl;
}

std::vector<std::pair<std::string, double>> calculate_wpms_by_player(
    const std::vector<std::string> &player_names,
    std::map<std::string, std::vector<double>> &mp_results_map,
    bool end_of_game = false)
// This function calculates the average WPM thus far for each
// player within a multiplayer game.
{
  std::vector<std::pair<std::string, double>> player_wpm_pairs;

  // Calculating WPMs for each player:
  for (auto &player : player_names) {
    if (mp_results_map[player].size() > 0) // Without this check,
    // a '-nan' entry will show in the output for players who
    // haven't yet performed their first round.
    { // The following std::accumulate() code was based on
      // https://en.cppreference.com/w/cpp/
      // algorithm/accumulate.html .
      double player_wpm_sum = std::accumulate(
          mp_results_map[player].begin(), mp_results_map[player].end(), 0.0);
      // Calculating the player's mean WPM, then adding it to our
      // vector of player WPM pairs:
      player_wpm_pairs.push_back(std::pair<std::string, double>{
          player, player_wpm_sum / mp_results_map[player].size()});
    }
  }

  // Sorting the results by WPM:
  // This sort() call uses a lambda function that allows pairs to
  // get sorted by their WPM values in descending order.
  // the code was based on the examples shown at
  // https://en.cppreference.com/w/cpp/algorithm/sort.html .
  std::sort(player_wpm_pairs.begin(), player_wpm_pairs.end(),
            [](std::pair<std::string, double> pair_1,
               std::pair<std::string, double> pair_2) {
              return pair_1.second > pair_2.second;
            });

  if (end_of_game == true) {
    Term::cout << "That concludes this multiplayer game. \
Here are the final WPM rankings:"
               << std::endl;
  }

  else {
    Term::cout << "That concludes the current player's turn for the \
current round. Here are the WPM rankings thus far: "
               << std::endl;
  }
  for (auto player_wpm : player_wpm_pairs) {
    Term::cout << player_wpm.first << ": " << player_wpm.second << std::endl;
  }

  if (end_of_game ==
      true) { // Printing the player with the highest average WPM: (Note: in the
    // unlikely event of a tie, this player won't be the only winner.)
    Term::cout << "Congratulations, " << player_wpm_pairs[0].first
               << "--you won!" << std::endl;
  }

  return std::move(player_wpm_pairs);
}

void run_multiplayer_game(std::string py_complement_name) {

  long test_number = 0; // Since there are no previous
  // tests to account for, we can initialize this value as 0.
  long session_number = 1; // This will always be 1 for
  // multiplayer games.
  int within_session_test_number = 0;

  // Determining the start time of this multiplayer session (which
  // will be incorporated into the name of the file that will store
  // its results):

  std::time_t unix_multiplayer_start_time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  char multiplayer_start_container[25]; // This value could likely
  // be reduced now that I've simplified the formatting instructions
  // (more on this below).

  // The following time formatting code does not use any
  // separators or time zone info in order to prevent invalid
  // filenames from being created on certain systems (and to
  // reduce the length of the output.)
  std::strftime(multiplayer_start_container, 25, "%Y%m%dT%H%M%S",
                std::localtime(&unix_multiplayer_start_time));
  std::string multiplayer_start_time_as_string = multiplayer_start_container;

  // Importing Bible verses:

  std::vector<Verse_Row> vrv = import_verses(verses_file_path);

  // Printing an initial welcome message:
  Term::cout << Term::clear_screen() << Term::terminal.clear()
             << Term::cursor_move(1, 1)
             << "Welcome to Type Through the Bible's multiplayer \
mode! First, enter the names of all players one by one; make sure \
to press Enter after each name. (Player names should not \
include whitespace.) Once all names have been \
entered, type 'c' and hit Enter."
             << std::endl;

  std::vector<std::string> player_names = {};

  std::string new_player_name;

  // The following while loop won't exit until (1) the player has
  // entered "c" AND (2) at least two players have been entered.
  while (!((new_player_name == "c") & (player_names.size() >= 1))) {
    new_player_name = cooked_input_within_raw_mode();
    if (new_player_name == "c") {
      if (player_names.size() < 1) {
        Term::cout << "\nPlease enter at least \
one player name." << std::endl;
      }
      continue;
    } // This check prevents 'c' from being added
    // as an additional player.
    player_names.push_back(new_player_name);
    Term::cout << "\nAdded " << new_player_name << " to player list. \
You may now enter the next player's name."
               << std::endl;
  }

  Term::cout << "\nHere are the " << player_names.size() << " players \
who will be joining this game:"
             << std::endl;
  for (int i = 0; i < player_names.size(); i++) {
    Term::cout << background_color_prefix + background_color_codes[i % 16] +
                      background_color_suffix
               << player_names[i] << std::endl;
  }

  // Determining how many rounds will be played, and how many
  // tests will be played (per player) each round:

  int multiplayer_rounds = 0;
  int tests_per_round = 0;

  while (true) // Since only certain entries will be valid,
  // we'll want to allow the player to retry his/her entries
  // if needed.
  {

    std::string multiplayer_rounds_as_string = "";
    std::string tests_per_round_as_string = "";

    Term::cout << "\nNext, type an integer that represents \
how many rounds you would like to play, then hit Enter."
               << std::endl;

    multiplayer_rounds_as_string = cooked_input_within_raw_mode();

    Term::cout << "\nAnd now please type, in integer form, \
how many consecutive typing tests each player will \
perform each round. Press Enter when finished."
               << std::endl;

    tests_per_round_as_string = cooked_input_within_raw_mode();
    try {
      multiplayer_rounds = std::stoi(multiplayer_rounds_as_string);
      tests_per_round = std::stoi(tests_per_round_as_string);
      // Making sure that the player isn't asking for more
      // verses than the Bible contains:
      if (multiplayer_rounds * tests_per_round > vrv.size()) {
        Term::cout << "\nPlease make sure that the product of your \
two integers doesn't exceed "
                   << vrv.size() << "." << std::endl;
        continue;
      }
      if ((multiplayer_rounds < 1) || (tests_per_round < 1)) {
        Term::cout << "\nMake sure to enter two \
positive integers." << std::endl;
        continue;
      }

      break;
    } catch (...) {
      Term::cout << "\nPlease enter two integers." << std::endl;
    }
  }

  Term::cout << "\nThis game will feature " << multiplayer_rounds
             << " round(s) with " << tests_per_round << " test(s) per round. \
With " << player_names.size()
             << " players, this game will \
include " << multiplayer_rounds * player_names.size() * tests_per_round
             << " tests in total." << std::endl;

  Term::cout << "\nNext, type a tag to incorporate into the \
filename that will store your game, then hit Enter. \
(This step is optional; hit Enter to skip it.) The tag should \
be 16 or fewer characters and should not have \
any spaces." << std::endl;

  std::string multiplayer_filename_string = (cooked_input_within_raw_mode());

  if (multiplayer_filename_string.size() > 16)
  // Keeping only 16 characters from this string (to prevent
  // larger-than-desired filename lengths):
  {
    multiplayer_filename_string = (multiplayer_filename_string.substr(0, 16));
  }

  if (multiplayer_filename_string == "CMR")
  // Since 'CMR' is used as a string for combined multiplayer
  // results, this entry will be changed to CMR1 just in case
  // a combined multiplayer response file with the same
  // initial timestamp will get saved to the file list. (This
  // is very unlikely, but better safe than sorry!)
  {
    Term::cout << "\nIn order to prevent another file from getting \
    overwritten, 'CMR1' will be used in place of 'CMR'."
               << std::endl;
    multiplayer_filename_string = "CMR1";
  }

  // Initializing filenames for test results, word results, and
  // a pivot table that will store players' average WPMs;

  std::string multiplayer_test_results_path =
      ("../Files/Multiplayer/" + multiplayer_start_time_as_string + "_" +
       multiplayer_filename_string + "_test_results.csv");

  std::string multiplayer_word_results_path =
      ("../Files/Multiplayer/" + multiplayer_start_time_as_string + "_" +
       multiplayer_filename_string + "_word_results.csv");

  std::string mp_pivot_path =
      ("../Files/Multiplayer/" + multiplayer_start_time_as_string + "_" +
       multiplayer_filename_string + "_pivot.csv");

  Term::cout << "\nThe full set of test- and word-level results \
from this session will be available at "
             << multiplayer_test_results_path << " and "
             << multiplayer_word_results_path
             << ", respectively; mean WPMs for \
each player will be stored at "
             << mp_pivot_path << "." << std::endl;

  // Determining the number of verses to trim off of the valid
  // verse ID range for the start of the multiplayer game:
  // (See select_verse_id documentation for more details.)

  int last_verse_offset =
      (multiplayer_rounds * tests_per_round) - 1; // Subtracting 1 here
  // will allow a multiplayer round to end on the final verse
  // within the Bible.

  Term::cout << "\n\nFinally, type the integer corresponding to \
the verse ID at which you would like to begin the game, followed \
by Enter. This can range from 1 to "
             << vrv.size() - last_verse_offset << ". (Type -1 to \
cancel this multiplayer session, and -2 to choose \
randomly-selected verses; hit Enter after those options also.)"
             << std::endl;

  int starting_verse_index_to_type =
      select_verse_id(vrv, multiplayer_rounds * tests_per_round);

  if (starting_verse_index_to_type == -1) {
    Term::cout << "\nCanceling multiplayer game." << std::endl;
    return;
  }

  std::vector<int> random_verse_ids{};

  if (starting_verse_index_to_type == -2) // In this case,
  // players will receive randomly-selected verses to type.
  {
    // The following code for generating random numbers was based
    // on https://en.cppreference.com/w/cpp/numeric/random.html
    // and p. 588 of the 3rd edition of Programming: Principles and
    // and Practices Using C++ (3rd Edition).

    std::random_device rd;
    std::default_random_engine dre(rd());
    for (int i = 0; i < multiplayer_rounds * tests_per_round; i++) {
      random_verse_ids.push_back(
          std::uniform_int_distribution{1, int(vrv.size())}(dre));
    }
  }

  // The following struct will facilitate the process of adding
  // relevant information (including player names) to our test
  // results file.

  Game_Config mgcf;

  bool marathon_mode = false;

  std::vector<Word_Result_Row> wrrv;
  std::vector<Test_Result_Row> trrv;
  // std::vector<Keypress_Processing_Time_Row> kptrv;

  std::map<std::string, std::vector<double>> mp_results_map;
  // Map that will be used to calculate players' average WPMs

  std::vector<std::pair<std::string, double>> player_wpm_pairs;
  // Vector (to be updated by calculate_wpms_by_player() function
  // that will keep track of each player's average WPM results
  // thus far.

  int verse_index_to_type = 1; // This is just a placeholder value;
  // the actual value will get selected prior to each test.

  for (int current_round = 1; current_round < (multiplayer_rounds + 1);
       current_round++) {
    for (int player_index = 0; player_index < player_names.size();
         player_index++) {
      for (int current_test_within_round = 1;
           current_test_within_round < (tests_per_round + 1);
           current_test_within_round++)

      {
        mgcf.player = player_names[player_index];
        mgcf.tag_1 = std::to_string(current_round);
        mgcf.tag_2 = std::to_string(current_test_within_round);
        // Tag 3 shows how many tests the current player
        // has completed (including the current test).
        mgcf.tag_3 = std::to_string((current_round - 1) * (tests_per_round) +
                                    (current_test_within_round));
        mgcf.notes = ""; // the 'notes' tag will remain blank
        // within multiplayer games, though you can always add
        // content to this column within the .csv file
        // after the fact.
        mgcf.mode = "MP"; // MP stands for 'multiplayer.'
        // If these results get added to a player's main
        // results file, it will be helpful to be able to
        // differentiate these tests from his/her
        // tests in single-player mode.

        // Determining which verse to present:
        if (starting_verse_index_to_type != -2) {
          verse_index_to_type = (starting_verse_index_to_type +
                                 (current_round - 1) * (tests_per_round) +
                                 (current_test_within_round - 1));
        } else {
          verse_index_to_type =
              random_verse_ids[(current_round - 1) * (tests_per_round) +
                               (current_test_within_round - 1)];
        }

        bool completed_test = false;
        while (completed_test == false) {
          // Note: I added in background-color codes below to help
          // highlight when a new player's session has begun.
          // (I also added in color codes for rounds and within-round
          // tests in order to liven up the display a little. :)
          Term::cout
              << "\n\nHere are the details for the \
following test:\nRound: "
              << background_color_prefix +
                     background_color_codes[(current_round - 1) % 16] +
                     background_color_suffix
              << " " << current_round << "\nPlayer: "
              << background_color_prefix +
                     background_color_codes[player_index % 16] +
                     background_color_suffix
              << " " << mgcf.player << "\nTest within round: "
              << background_color_prefix +
                     background_color_codes[(current_test_within_round - 1) %
                                            16] +
                     background_color_suffix
              << " " << current_test_within_round << std::endl;
          // The following keypress prompt was added in so that players
          // will be able to view information about WPM stats and
          // the next test before run_test() gets called. (That function
          // clears the screen, so without this prompt, players wouldn't
          // have a chance to see this information.)
          Term::cout << "Press 'c' to \
continue. (The test won't start just yet.)"
                     << std::endl;
          // bool proceed_to_test = false;
          // while (proceed_to_test == false)
          //     {
          //         Term::Event event = Term::read_event();
          //         switch (event.type())
          //         {
          //         case Term::Event::Type::Key:
          //         {
          //             Term::Key key(event);
          //             proceed_to_test = true;
          //             break;
          //         }
          //         default:
          //             break;
          //         }
          //     };
          get_single_keypress({"c"});

          // The following loop will continue until a player
          // has successfully completed a test.
          std::string within_test_update_message = "";
          std::string post_entry_message = ""; // No post-entry message
          // is necessary within multiplayer games, as such messages are
          // only shown within the two single-player marathon modes.

          completed_test = run_test(
              vrv[verse_index_to_type], trrv, wrrv, marathon_mode, mgcf.player,
              mgcf.mode, mgcf.tag_1, mgcf.tag_2, mgcf.tag_3, mgcf.notes,
              test_number, session_number, within_session_test_number, true,
              // kptrv,
              within_test_update_message, post_entry_message);
          // The 'true' argument here governs the
          // 'allow_quitting' parameter. I had
          // originally set it to False, but realized
          // during testing that this could cause
          // major issues (i.e. if a player begins a
          // test by accident and isn't ready to complete
          // it.)
        }
        // Storing the player's WPM result (which can be
        // identified as the most recent WPM entry
        // within trrv) within the WPM vector corresponding
        // to his/her name in mp_results_map:
        mp_results_map[mgcf.player].push_back(trrv.back().wpm);
      }
      // Calculating and reporting average WPMs for each player
      // thus far:
      // Checking whether we just finished the game: (In this case,
      // we'll want to pass this info on to
      // calculate_wpms_by_player() so that that function can
      // announce the winner of the game.)
      bool end_of_game = false;
      if ((current_round == multiplayer_rounds) &&
          (player_index == player_names.size() - 1)) {
        end_of_game = true;
      }
      player_wpm_pairs =
          calculate_wpms_by_player(player_names, mp_results_map, end_of_game);
      // Consider adding code here that would allow players to
      // finish the game early.
    }
  }

  // Exporting test results:
  // Note that, because these test and word result files are
  // new documents, header rows will need to be added in for
  // both of them.

  export_test_results(trrv, multiplayer_test_results_path, true, false);

  // Exporting word results:

  export_word_results(wrrv, multiplayer_word_results_path, true, false);

  // Exporting player_wpm_pairs data to a .csv file:

  auto mp_pivot_export_start_time = std::chrono::high_resolution_clock::now();

  std::ofstream mp_pivot_ofstream{mp_pivot_path};
  auto mp_pivot_writer = make_csv_writer(mp_pivot_ofstream);

  std::vector<std::string> header_row = {"Player", "Mean_WPM"};

  // Writing this header to the .csv file:
  mp_pivot_writer << header_row;

  for (int i = 0; i < player_wpm_pairs.size(); ++i) {
    std::vector<std::string> cols_as_strings = {
        player_wpm_pairs[i].first, std::to_string(player_wpm_pairs[i].second)};
    mp_pivot_writer << cols_as_strings;
  };

  auto mp_pivot_export_end_time = std::chrono::high_resolution_clock::now();
  auto multiplayer_export_seconds =
      std::chrono::duration<double>(mp_pivot_export_end_time -
                                    mp_pivot_export_start_time)
          .count();
  Term::cout << "Exported " << player_wpm_pairs.size() << " mean WPM rows in "
             << multiplayer_export_seconds << " seconds." << std::endl;

  // Defining a system call as a std::string:
  // The following code was based on
  // https://stackoverflow.com/a/4907852/13097194

  Term::cout << "Type 'y' to call a Python script that will visualize \
your new multiplayer stats; type 'n' to skip this process. \
(This may require some setup on your part; \
see Readme for more details. You can also run the multiplayer script \
separately if you prefer."
             << std::endl;

  std::string mp_stats_update_response = "";

  mp_stats_update_response = get_single_keypress({"y", "n"});

  if (mp_stats_update_response == "y") {

    Term::cout << "Calling Python script. (This can take \
a little while.)"
               << std::endl;

    std::string system_call =
        (py_complement_name + " mpv " + multiplayer_start_time_as_string);
    // For security purposes, only the timestamp (rather than
    // the user-provided string that makes up part of the filename)
    // will get passed to the following system() call. (The
    // Python script will be able to locate the correct multiplayer
    // results file based on this timestamp alone.)

    try {
      system(system_call.c_str());
    } catch (...) {
      Term::cout << "Unable to run system command." << std::endl;
    }
  }

  else {
    Term::cout << "Skipping multiplayer visualizations." << std::endl;
  }
}

void import_mp_results() {
  Term::cout << "This option will allow you to import your portion \
of a multiplayer results file into your main single-player results \
(i.e. the ones stored in your Files/ folder.\nFirst, enter the name \
of the multiplayer file timestamp and string whose data you wish \
to import. These items can be found within your main multiplayer \
test results folder. For instance, if your full multiplayer test \
results filename is 20250723T224940_testmpspimport_test_results.csv, \
enter just 20250723T224940_testmpspimport ."
             << std::endl;

  std::string mp_timestamp_and_tag = cooked_input_within_raw_mode();

  Term::cout << "\nNext, enter the EXACT name of the player, as shown \
within the multiplayer results file, whose results you wish to \
import into your main single-player file."
             << std::endl;

  std::string player_to_import = cooked_input_within_raw_mode();

  // Identifying the files from which we will import multiplayer
  // results:

  std::string mp_test_results_filename =
      ("../Files/Multiplayer/" + mp_timestamp_and_tag + "_test_results.csv");

  std::string mp_word_results_filename =
      ("../Files/Multiplayer/" + mp_timestamp_and_tag + "_word_results.csv");

  // Reading multiplayer test results into memory:

  // Checking how many test results have been completed so far:
  // (We'll need this information in order to determine what new
  // test numbers to assign our files. Also note that, depending on
  // when this function was called, these test numbers might be
  // greater than those of earlier single-player tests.)
  // We'll also import the highest session number within
  // our test result so that we can initialize these results'
  // session number as the integer one greater than that
  // number.

  std::map<std::string, long> test_and_session_numbers =
      count_sp_test_results();
  long test_number = test_and_session_numbers["test_number"];
  long session_number = test_and_session_numbers["session_number"] + 1;

  // For debugging purposes
  // Term::cout << test_number << " tests have been completed so far."
  // << std::endl;

  std::vector<Test_Result_Row> original_mp_trrv =
      import_test_results(mp_test_results_filename);

  Term::cout << "\nFinished importing multiplayer test result data \
from:\n" << mp_test_results_filename
             << std::endl;

  // Loading Bible verse data: (This file will get updated with the
  // player's multiplayer results.)

  std::vector<Verse_Row> vrv = import_verses(verses_file_path);

  // Loading game configuration settings: (These will override the
  // existing Player, Mode, Tag_1, Tag_2, Tag_3, and Notes values within
  // the multiplayer results file.

  Game_Config new_gcf = initialize_game_config("MP");

  Term::cout << "The existing tags and player values in the \
multiplayer results file will get overwritten with these values. \
To update these values before they get saved to your single-player \
file, enter 'y'; to keep them as they are, enter 'n'."
             << std::endl;

  std::string config_update_request = get_single_keypress({"y", "n"});

  if (config_update_request == "y")

  {
    update_game_config(new_gcf);
  }

  Term::cout << "Data for " << player_to_import << " will now be \
added to your main test results files under the name "
             << new_gcf.player << ". Press 'y' to proceed or \
'n' to cancel."
             << std::endl;

  std::string import_confirmation = get_single_keypress({"y", "n"});

  if (import_confirmation == "y") {

    // Creating a variant of mp_trrv that will include only the rows
    // whose Player values are equal to player_to_import (and whose
    // tag values will equal those requested by the player):
    // (Results from this vector will get added to our single-player
    // test_results.csv file.)

    std::vector<Test_Result_Row> mp_trrv_for_import;

    // The following map will store original multiplayer test numbers
    // (not within-session test numbers) as keys and their corresponding
    // single-player test numbers as values. This will allow us to
    // determine (1) which word-level results to add to
    // our single-player word results, and (2) which test number values
    // to assign those word-level results.
    std::map<long, long> mp_to_sp_test_number_map;

    for (auto trr : original_mp_trrv) {
      if (trr.player == player_to_import) // We'll only want to add
      // rows for the player found in player_to_import to our
      // single-player results file--hence this check.
      // Creating a copy of trr that we can update to better align
      // with our single-player results:
      {
        Test_Result_Row trr_for_import = trr;
        // Storing the single-player test number that corresponds
        // to the original multiplayer test number value:
        // Note: within multiplayer files, Tag_3 represents the
        // number of tests (including the current test) the player
        // has completed thus far; thus, it can be used as
        // the within-session test number. In addition, to calculate
        // the overall test number, we can simply add this Tag_3
        // value to the test_number value generated using
        // count_sp_test_results().

        mp_to_sp_test_number_map[trr_for_import.test_number] =
            (test_number + std::stoi(trr_for_import.tag_3));

        // Updating the player name to match that within new_gcf:
        trr_for_import.player = new_gcf.player;
        // Note that the 'Mode' value will remain multiplayer (since
        // these tests were originally completed within a
        // multiplayer game.)

        // Updating the test numbers and within-session test numbers
        // to align with those found in the single-player results
        // file:

        trr_for_import.test_number =
            (test_number + std::stoi(trr_for_import.tag_3));
        trr_for_import.session_number = session_number;
        trr_for_import.within_session_test_number =
            std::stoi(trr_for_import.tag_3);
        // Now that we've utilized this tag_3 value, we can
        // overwrite it (along with the other tags) with the values
        // found within new_gcf.
        trr_for_import.player = new_gcf.player;
        trr_for_import.tag_1 = new_gcf.tag_1;
        trr_for_import.tag_2 = new_gcf.tag_2;
        trr_for_import.tag_3 = new_gcf.tag_3;
        trr_for_import.notes = new_gcf.notes;

        // For debugging
        // Term::cout << "Updated values + WPM:" <<
        // trr_for_import.test_number << "\t" <<
        // trr_for_import.within_session_test_number << "\t" <<
        // trr_for_import.wpm << "\t" <<
        // trr_for_import.verse_code << "\t" <<
        // trr_for_import.player << "\t" <<
        // trr_for_import.tag_1 << "\t" <<
        // trr_for_import.tag_2 << "\t" <<
        // trr_for_import.tag_3 << "\t" <<
        // trr_for_import.notes << "\t" << std::endl;

        mp_trrv_for_import.push_back(trr_for_import);

        // Updating Bible verse file to reflect this test:
        // (As a reminder, verse indices are always one less
        // than verse IDs, as the former start at 0 and the
        // latter start at 1.

        vrv[trr_for_import.verse_id - 1].tests += 1;
        if (trr_for_import.wpm > vrv[trr_for_import.verse_id - 1].best_wpm) {
          vrv[trr_for_import.verse_id - 1].best_wpm = (trr_for_import.wpm);
        }
      }
    }

    export_test_results(mp_trrv_for_import, "../Files/test_results.csv", false,
                        false);

    Term::cout << "A revised copy of your multiplayer test result \
data was added to test_results.csv."
               << std::endl;

    export_verses(vrv, verses_file_path);

    Term::cout << "Your Bible verse file was also updated with the \
results of your multiplayer tests."
               << std::endl;

    // Adding the player's word-level multiplayer results to the
    // single-player word-level results file:

    std::vector<Word_Result_Row> original_mp_wrrv =
        import_word_results(mp_word_results_filename);

    std::vector<Word_Result_Row> mp_wrrv_for_import;

    Term::cout << "Finished importing multiplayer word result data \
from:\n" << mp_test_results_filename
               << std::endl;

    for (auto wrr : original_mp_wrrv)

    { // Using the multiplayer test number/single-player test number
      // map we created earlier to determine whether this given result
      // was typed by the player whose multiplayer results we wish to copy
      // into the single-player ones:
      if (mp_to_sp_test_number_map.find(wrr.test_number) !=
          mp_to_sp_test_number_map.end()) // Based on p. 584 of PPP3.
      // A C++-20 solution, shared by Denis Sablukov, can be found here:
      // https://stackoverflow.com/a/54200516/13097194
      {
        Word_Result_Row wrr_for_import = wrr;
        // Replacing the original multiplayer test number with
        // one that will correspond to other single-player numbers:
        long new_wrr_test_number =
            mp_to_sp_test_number_map[wrr_for_import.test_number];
        wrr_for_import.test_number = new_wrr_test_number;
        mp_wrrv_for_import.push_back(wrr_for_import);
      }
    }

    export_word_results(mp_wrrv_for_import, "../Files/word_results.csv", false,
                        false);

    Term::cout << "A revised copy of your multiplayer word result \
data was added to word_results.csv. Multiplayer imports are now \
complete! However, visualizations of your single-player results will \
not be updated until you next run TTTB in single-player mode.\n"
               << std::endl;

  }

  else {
    Term::cout << "Import canceled. Your single-player files were \
not modified." << std::endl;
  }
}

void combine_multiplayer_results(std::string py_complement_name)
// This function calls a Python script that (1) combines multiple
// sets of multiplayer results into the same file, then (2)
// calls another Python script to create visualizations of those
// results.
{
  Term::cout << "Type the first and last verse IDs (inclusive) of \
the multiplayer session, separated by an underscore, followed \
by Enter. (Example \
input: 30786_30795) Only results within \
this range of IDs will get incorporated into the \
results.\n\nAlternatively, type 'y', followed by 'Enter', to \
have the game detect these IDs within the first set of results \
that it \
analyzes." << std::endl;

  std::string verse_ids = cooked_input_within_raw_mode();

  Term::cout << "\nType 'y' to process these results and 'n' to \
cancel." << std::endl;

  std::string result_combination_confirmation =
      (get_single_keypress({"y", "n"}));

  if (result_combination_confirmation == "y")

  {
    Term::cout << "Calling Python script. (This can take \
a little while.)"
               << std::endl;
    std::string system_call = py_complement_name + " mpfc " + verse_ids;

    try {
      system(system_call.c_str());
    } catch (...) {
      Term::cout << "Unable to run system command." << std::endl;
    }
  }

  else {
    Term::cout << "Canceling operation. No files were modified." << std::endl;
  }

  Term::cout << std::endl;
}

int main() {
  // Determining which executable command to pass to system calls:
  // One name should work for both Linux and OSX; the other name
  // should work for Windows.

  std::string py_complement_name = "";
#ifdef _WIN32 // From
  // https://www.geeksforgeeks.org/cpp/writing-os-independent-code-cc/
  {
    py_complement_name = "tttb_py_complement.exe";
  }
#else
  {
    py_complement_name = "./tttb_py_complement";
  }
#endif

  // Printing a simple graphic (which will likely look different
  // on different terminals) along with a welcome message:
  for (int i = 0; i < background_color_codes.size(); i++) {
    Term::cout << background_color_prefix + background_color_codes[i] +
                      background_color_suffix;
  }
  Term::cout << std::endl
             << "Welcome to Version 1.02 of \
Type Through the Bible (C++ Edition)!\nPlease review the README.pdf \
file for important gameplay instructions."
             << std::endl;

  std::string game_exit_message = "Exiting Type Through \
the Bible.";

  std::string gameplay_option = "0";
  while (gameplay_option != "e") {
    // Initializing our terminal:
    /*The following code was based in part on
    https://github.com/jupyter-xeus/cpp-terminal/blob/
    master/examples/events.cpp .*/

    // (I moved this code within the while loop so that the terminal
    // can get reinstated following certain system() calls
    // that print out Python-based output.)
    Term::terminal.setOptions(Term::Option::NoClearScreen,
                              Term::Option::NoSignalKeys,
                              Term::Option::NoCursor, Term::Option::Raw);
    // The following line comes from the keys.cpp example
    // at
    // https://github.com/jupyter-xeus/cpp-terminal/blob/master/examples/keys.cpp

    if (!Term::is_stdin_a_tty()) {
      throw Term::Exception(
          "The terminal is not attached to a TTY and therefore \
can't catch user input. Exiting...");
    }

    // The following code was helpful in developing and debugging
    // both the cooked_input_within_raw_mode() and the
    // get_single_keypress() functions.
    // std::string cooked_input_output = "";
    // while (cooked_input_output != "e")
    {
      // cooked_input_output = cooked_input_within_raw_mode(
      //     "Please enter a string", true);
      // Term::cout << "The string you entered was:" << cooked_input_output <<
      // std::endl;
      //

      // cooked_input_output = get_single_keypress({
      //     "s", "m", "r", "c", "e"});
      // Term::cout << "The string you entered was: "
      // << cooked_input_output << std::endl;
    }

    Term::cout << "\nType 's' or 'm' for single-player or \
multiplayer mode, respectively.\nTo import multiplayer results \
into your single-player files, type 'r.'\nTo combine multiplayer \
results within ..Files/MP_Test_Result_Files_To_Combine into a single \
file, then analyze them, type 'c'.\nTo exit the game, \
type 'e.'" << std::endl;

    gameplay_option = get_single_keypress({"s", "m", "r", "c", "e"});

    if (gameplay_option == "s") {
      run_single_player_game(py_complement_name);
      continue;
    }

    else if (gameplay_option == "m") {
      run_multiplayer_game(py_complement_name);
      continue;
    }

    else if (gameplay_option == "e") {
      Term::cout << "Exiting Type Through the Bible." << std::endl;
      continue;
    }

    else if (gameplay_option == "r") {
      import_mp_results();
      continue;
    }

    else if (gameplay_option == "c") {
      combine_multiplayer_results(py_complement_name);
      continue;
    }

    else {
      Term::cout << "That input wasn't recognized. \
Please try again."
                 << std::endl;
    }
  }
}