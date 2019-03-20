#include <gtest/gtest.h>
#include "mcnp_funcs.h"

#include <cmath>
#include <cassert>

#include <thread>

std::string test_file = "test_geom_legacy.h5m";
std::string test_file_comp = "test_geom_legacy_comp.h5m";
std::string test_file_reflecting = "test_reflecting.h5m";

//---------------------------------------------------------------------------//
// TEST FIXTURES
//---------------------------------------------------------------------------//
class DAGMCNP5Test : public ::testing::Test {
 public:
  void setup_problem() {
    std::string filename = test_file;
    char* file = &filename[0];
    int len = filename.length();
    std::string facet_tol = "1.0e-4";
    char* ftol = &facet_tol[0];
    int ftol_len = facet_tol.length();
    int parallel_mode = 0;
    double dagmc_version;
    int moab_version;
    int max_pbl = 0;

    // intialise dagmc
    dagmcinit_(file, &len, ftol, &ftol_len, &parallel_mode,
               &dagmc_version, &moab_version, &max_pbl);

  }
  void setup_problem_comp() {
    std::string filename = test_file_comp;
    char* file = &filename[0];
    int len = filename.length();
    std::string facet_tol = "1.0e-4";
    char* ftol = &facet_tol[0];
    int ftol_len = facet_tol.length();
    int parallel_mode = 0;
    double dagmc_version;
    int moab_version;
    int max_pbl = 0;

    // intialise dagmc
    dagmcinit_(file, &len, ftol, &ftol_len, &parallel_mode,
               &dagmc_version, &moab_version, &max_pbl);

  }

  void setup_fire_ray() {
    std::string filename = test_file;
    char* file = &filename[0];
    int len = filename.length();
    std::string facet_tol = "1.0e-4";
    char* ftol = &facet_tol[0];
    int ftol_len = facet_tol.length();
    int parallel_mode = 0;
    double dagmc_version;
    int moab_version;
    int max_pbl = 0;

    // intialise dagmc
    dagmcinit_(file, &len, ftol, &ftol_len, &parallel_mode,
               &dagmc_version, &moab_version, &max_pbl);

    int ih = 1;
    double u = 0.701, v = 0.7071, w = 0.0;
    double x = 0.0, y = 0.0, z = 0.0;
    double huge = 1.E37;
    double dist{};

    int jap{};
    int jsu{};
    int nps = 1;

    dagmctrack_(&ih, &u, &v, &w, &x, &y, &z, &huge, &dist, &jap, &jsu, &nps);

    EXPECT_TRUE(dist < huge);
    EXPECT_TRUE(jap != 0);

  }



  void setup_problem_reflecting() {
    std::string filename = test_file_reflecting;
    char* file = &filename[0];
    int len = filename.length();
    std::string facet_tol = "1.0e-4";
    char* ftol = &facet_tol[0];
    int ftol_len = facet_tol.length();
    int parallel_mode = 0;
    double dagmc_version;
    int moab_version;
    int max_pbl = 0;

    // intialise dagmc
    dagmcinit_(file, &len, ftol, &ftol_len, &parallel_mode,
               &dagmc_version, &moab_version, &max_pbl);

  }

};

// Test setup outcomes
TEST_F(DAGMCNP5Test, dagmclcad_test) {
  setup_problem();
}

// Test setup outcomes
TEST_F(DAGMCNP5Test, dagmcinit_test) {
// expected values from the lcad file // only the cells
  const char* expected[] = {"1 9 -12.0 imp:n=1 imp:p=1 ",
                            "2 9 -12.0 imp:n=1 imp:p=1 ",
                            "3 9 -12.0 imp:n=1 imp:p=1 ",
                            "4 9 -12.0 imp:n=1 imp:p=1 ",
                            "5 3 -2.0 imp:n=1 imp:p=1 ",
                            "6 3 -2.0 imp:n=1 imp:p=1 ",
                            "7 3 -2.0 imp:n=1 imp:p=1 ",
                            "8 3 -2.0 imp:n=1 imp:p=1 ",
                            "9 1 0.022 imp:n=1 imp:p=1 ",
                            "12 0  imp:n=0 imp:p=0   $ graveyard",
                            "13 0  imp:n=1 imp:p=1   $ implicit complement"
                           };
  std::vector<std::string> expected_lcad(expected, expected + 11);

  std::string dagfile = test_file;
  char* dfile = &test_file[0];
  std::string lfile = "lcad";
  char* lcadfile = &lfile[0];
  int llen = 4;
  dagmcwritemcnp_(dfile, lcadfile, &llen);

  // now read the lcad file
  std::ifstream input;
  input.open("lcad");
  std::string line;
  std::vector<std::string> input_deck;
  while (!input.eof()) {
    std::getline(input, line);
    input_deck.push_back(line);
  }
  input.close();

  // for each line make sure the same
  for (int i = 0 ; i < 11 ; i++) {
    EXPECT_EQ(expected_lcad[i], input_deck[i]);
  }
  // delete the lcad file
  std::remove("lcad");

  // clearout the dagmc instance
  dagmc_teardown_();
}

// Test setup outcomes
TEST_F(DAGMCNP5Test, dagmclcad_comp_test) {
  setup_problem_comp();
}

// Test setup outcomes
TEST_F(DAGMCNP5Test, dagmcinit_comp_test) {
// expected values from the lcad file // only the cells
  const char* expected[] = {"1 9 -12.0 imp:n=1 imp:p=1 ",
                            "2 9 -12.0 imp:n=1 imp:p=1 ",
                            "3 9 -12.0 imp:n=1 imp:p=1 ",
                            "4 9 -12.0 imp:n=1 imp:p=1 ",
                            "5 3 -2.0 imp:n=1 imp:p=1 ",
                            "6 3 -2.0 imp:n=1 imp:p=1 ",
                            "7 3 -2.0 imp:n=1 imp:p=1 ",
                            "8 3 -2.0 imp:n=1 imp:p=1 ",
                            "9 1 0.022 imp:n=1 imp:p=1 ",
                            "12 0  imp:n=0 imp:p=0   $ graveyard",
                            "13 2 -3.1 imp:n=1 imp:p=1   $ implicit complement"
                           };
  std::vector<std::string> expected_lcad(expected, expected + 11);

  std::string dagfile = test_file;
  char* dfile = &test_file[0];
  std::string lfile = "lcad";
  char* lcadfile = &lfile[0];
  int llen = 4;
  dagmcwritemcnp_(dfile, lcadfile, &llen);

  // now read the lcad file
  std::ifstream input;
  input.open("lcad");
  std::string line;
  std::vector<std::string> input_deck;
  while (!input.eof()) {
    std::getline(input, line);
    input_deck.push_back(line);
  }
  input.close();

  // for each line make sure the same
  for (int i = 0 ; i < 11 ; i++) {
    EXPECT_EQ(expected_lcad[i], input_deck[i]);
  }
  // delete the lcad file
  std::remove("lcad");

  // clearout the dagmc instance
  dagmc_teardown_();
}
// Test setup outcomes

// Test setup outcomes
TEST_F(DAGMCNP5Test, dagmcreflecting_comp_test) {
  setup_problem_reflecting();
}


TEST_F(DAGMCNP5Test, dagmcreflecting_test) {
// expected values from the lcad file // only the cells
  const char* expected[] = {"1 1 -1.0 imp:n=1",
                            "2 0  imp:n=1  $ implicit complement",
                            "",
                            "*1",
                            "2",
                            "3",
                            "4",
                            "5",
                            "*6"
                           };
  int num_lines = 9;
  std::vector<std::string> expected_lcad(expected, expected + num_lines);

  std::string dagfile = test_file_reflecting;
  char* dfile = &test_file[0];
  std::string lfile = "lcad";
  char* lcadfile = &lfile[0];
  int llen = 4;
  dagmcwritemcnp_(dfile, lcadfile, &llen);

  // now read the lcad file
  std::ifstream input;
  input.open("lcad");
  std::string line;
  std::vector<std::string> input_deck;
  while (!input.eof()) {
    std::getline(input, line);
    input_deck.push_back(line);
  }
  input.close();

  // for each line make sure the same
  for (int i = 0 ; i < num_lines ; i++) {
    EXPECT_EQ(expected_lcad[i], input_deck[i]);
  }
  // delete the lcad file
  std::remove("lcad");

  // clearout the dagmc instance
  dagmc_teardown_();
}

// Test setup outcomes
TEST_F(DAGMCNP5Test, dagmc_fire_ray_test) {
  setup_fire_ray();
}
