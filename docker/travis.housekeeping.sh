#!/bin/bash

set -e

source ${docker_env}

cd ${dagmc_build_dir}/DAGMC


# Check for news file if this is a PR into svalinn/DAGMC
if [ "${TRAVIS_REPO_SLUG}" == "svalinn/DAGMC" ] && \
   [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then
  news_file=$(printf 'news/PR-%04u.rst' ${TRAVIS_PULL_REQUEST})
  if [ -f "${news_file}" ]; then
    echo "News file ${news_file} found!"
  else
    echo "ERROR: News file ${news_file} not found. Please create a news file."
    exit 1
  fi
fi

# Run astyle check
astyle --options=astyle_google.ini \
       --exclude=src/gtest \
       --exclude=src/mcnp/mcnp5/Source \
       --exclude=src/mcnp/mcnp6/Source \
       --ignore-exclude-errors \
       --suffix=none \
       --recursive \
       --verbose \
       --formatted \
       "*.cc" "*.cpp" "*.h" "*.hh" "*.hpp"
astyle_status=`git status --porcelain`
astyle_diff=`git diff`
if [ -z "${astyle_status}" ]; then
  echo "Style guide checker passed!"
else
  echo "ERROR: Style guide checker failed. Please run astyle."
  echo "Files changed:"
  echo "${astyle_status}"

  diff_line=$(echo $astyle_diff | wc -l)
  if [ $diff_lines -ge 100 ]; then
      echo "Diff is too large. Please run astyle on your repo."
  else
      echo "Diff:"
      # need to re-run to maintain formatting
      git diff | cat
  fi


  exit 1
fi

# Build documentation
make html

exit 0
