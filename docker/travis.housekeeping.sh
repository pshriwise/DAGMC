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

./scripts/apply_style_guide.sh

astyle_status=`git status --porcelain`
astyle_diff=`git diff`
if [ -z "${astyle_status}" ]; then
    echo "Style guide checker passed!"
else
    echo "ERROR: Style guide checker failed."
    echo "Files changed:"
    echo "${astyle_status}"

    # if the diff is small, display it
    n_diff_lines=$(echo $astyle_diff | wc -l)
    if [ $n_diff_lines -ge 50 ]; then
        echo "Diff is too large to show here"
    else
        echo "Diff:"
        # need to re-run to maintain formatting
        git diff | cat
    fi

    echo "Please apply the style guide using /scripts/apply_style_guide.sh"
    echo "from the root directory of the DagMC repo."

  exit 1
fi

# Build documentation
make html

exit 0
