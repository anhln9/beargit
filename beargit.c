#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>

#include "beargit.h"
#include "util.h"

/* Implementation Notes:
 *
 * - Functions return 0 if successful, 1 if there is an error.
 * - All error conditions in the function description need to be implemented
 *   and written to stderr. We catch some additional errors for you in main.c.
 * - Output to stdout needs to be exactly as specified in the function description.
 * - Only edit this file (beargit.c)
 * - You are given the following helper functions:
 *   * fs_mkdir(dirname): create directory <dirname>
 *   * fs_rm(filename): delete file <filename>
 *   * fs_mv(src,dst): move file <src> to <dst>, overwriting <dst> if it exists
 *   * fs_cp(src,dst): copy file <src> to <dst>, overwriting <dst> if it exists
 *   * write_string_to_file(filename,str): write <str> to filename (overwriting contents)
 *   * read_string_from_file(filename,str,size): read a string of at most <size> (incl.
 *     NULL character) from file <filename> and store it into <str>. Note that <str>
 *     needs to be large enough to hold that string.
 *  - You NEED to test your code. The autograder we provide does not contain the
 *    full set of tests that we will run on your code. See "Step 5" in the homework spec.
 */

/* beargit init
 *
 * - Create .beargit directory
 * - Create empty .beargit/.index file
 * - Create .beargit/.prev file containing 0..0 commit id
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_init(void) {
  fs_mkdir(".beargit");

  FILE* findex = fopen(".beargit/.index", "w");
  fclose(findex);

  FILE* fbranches = fopen(".beargit/.branches", "w");
  fprintf(fbranches, "%s\n", "master");
  fclose(fbranches);
   
  write_string_to_file(".beargit/.prev", "0000000000000000000000000000000000000000");
  write_string_to_file(".beargit/.current_branch", "master");

  return 0;
}



/* beargit add <filename>
 * 
 * - Append filename to list in .beargit/.index if it isn't in there yet
 *
 * Possible errors (to stderr):
 * >> ERROR: File <filename> already added
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_add(const char* filename) {
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      fprintf(stderr, "ERROR: File %s already added\n", filename);
      fclose(findex);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 3;
    }

    fprintf(fnewindex, "%s\n", line);
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}

int beargit_status() {

  FILE* findex = fopen(".beargit/.index", "r");
  int filecount = 0;

  fprintf(stdout, "Tracked files:\n\n");

  char line[FILENAME_SIZE];
  while (fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    fprintf(stdout, "%s\n", line);
    filecount += 1;
  }
  fprintf(stdout, "\n%d files total\n", filecount);
  fclose(findex);

  return 0;
}

int beargit_rm(const char* filename) {

  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");
  char line[FILENAME_SIZE];
  int filedeleted = 0;

  while (fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      filedeleted = 1;
    } else {
      fprintf(fnewindex, "%s\n", line);
    }
  }

  if (filedeleted == 0) {
    fprintf(stderr, "ERROR: File %s is not tracked\n", filename);
    fclose(findex);
    fclose(fnewindex);
    fs_rm(".beargit/.newindex");
    return 1;
  } else {
    fclose(findex);
    fclose(fnewindex);
    fs_mv(".beargit/.newindex", ".beargit/.index");
    return 0;
  }
}

/* beargit commit -m <msg> */

const char* go_bears = "GO BEARS!";

int is_commit_msg_ok(const char* msg) {

  /* Runs msg and go_bears in parallel to find similarity */
  int i, j;
  i = j = 0;
  int bear_length = strlen(go_bears);
  while (msg[i] != '\0') {
    if (msg[i] == go_bears[j]) {
      i++;
      j++;
    } else {
      if (j > 0 && (msg[i] != go_bears[j])) {
        // if checked is over the length of go_bears and still not same, reset j
        // and check again
        j = 0;
      } else {
        i++;
      }
    }
    if (j == bear_length) {
      return 1;
    }
  }
  return 0;
}

void next_commit_id_part1(char* commit_id) {

  int r, i;
  char c;
  int n = COMMIT_ID_BYTES - COMMIT_ID_BRANCH_BYTES; //40-10=30
  int num = 0;
  int base3 = 1;
  for (i = 0; i < n; i++) {
    c = commit_id[i];
    if (c == '6') {
      num += 0 * base3;
    } else if (c == '1') {
      num += 1 * base3;
    } else {
      num += 2 * base3;
    }
    base3 *= 3;
  }
  num++;

  for (i = 0; i < n; ++i) {
    r = num % 3;
    if (r == 0) {
      commit_id[i] = '6';
    } else if (r == 1) {
      commit_id[i] = '1';
    } else {
      commit_id[i] = 'c';
    }
    num /= 3;
  }

}


int beargit_commit(const char* msg) {
  if (!is_commit_msg_ok(msg)) {
    fprintf(stderr, "ERROR: Message must contain \"%s\"\n", go_bears);
    return 1;
  }

  char commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  next_commit_id(commit_id); 

  // Safety check, must be on HEAD of a branch to commit
  // Can't commit a random commit_id
  char current_branch[BRANCHNAME_SIZE];
  read_string_from_file(".beargit/.current_branch", current_branch, BRANCHNAME_SIZE);

  if (!strlen(current_branch)) {
    fprintf(stderr, "ERROR: Need to be on HEAD of a branch to commit\n");
    return 1;
  }

  // Create new commit directory, copy current .index, .prev to commit_dir
  // Create new .msg in commit_dir
  char commit_dir[80];
  char new_index_dir[80];
  char new_prev_dir[80];
  char new_msg_dir[80];
  sprintf(commit_dir, ".beargit/%s/", commit_id); //.beargit/6666.6/
  sprintf(new_index_dir, "%s.index", commit_dir); //.beargit/6666.6/.index
  sprintf(new_prev_dir, "%s.prev", commit_dir); //.beargit/6666.6/.prev
  sprintf(new_msg_dir, "%s.msg", commit_dir); //.beargit/6666.6/.msg

  fs_mkdir(commit_dir);
  fs_cp(".beargit/.index", new_index_dir);
  fs_cp(".beargit/.prev", new_prev_dir);
  write_string_to_file(".beargit/.prev", commit_id);
  write_string_to_file(new_msg_dir, msg);

  // Move all tracked files to commit_dir
  FILE * findex = fopen(".beargit/.index", "r");
  char line[FILENAME_SIZE];
  char new_dir[80];
  while (fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    strcpy(new_dir, commit_dir);
    fs_cp(line, strcat(new_dir, line));
  }
  fclose(findex);

  return 0;
}

int beargit_log(int limit) {

  int i, n;
  char previous_commit[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", previous_commit, COMMIT_ID_SIZE);

  char msg[80];
  char prev_dir[80];
  char msg_dir[80];

  fprintf(stdout, "\n");

  if (limit < INT_MAX) {
    n = limit;
  } else {
    n = INT_MAX;
  }
  for (i = 0; i < n; i++) {

    if (!fs_check_dir_exists(previous_commit)) {
      if ((i == 0) && (!strcmp(previous_commit, "0000000000000000000000000000000000000000"))) {
        fprintf(stderr, "ERROR: There are no commits!\n");
        return 1;
      } else if ((i > 0) && (!strcmp(previous_commit, "0000000000000000000000000000000000000000"))) {
        return 0;
      }
      fprintf(stdout, "commit %s\n", previous_commit);
      sprintf(msg_dir, ".beargit/%s/.msg", previous_commit);
      read_string_from_file(msg_dir, msg, 80);
      fprintf(stdout, "\t%s\n\n", msg);
      sprintf(prev_dir, ".beargit/%s/.prev", previous_commit);
      read_string_from_file(prev_dir, previous_commit, COMMIT_ID_SIZE);
    }
  }
  return 0;
}


const char* digits = "61c";

void next_commit_id(char* commit_id) {
  char current_branch[BRANCHNAME_SIZE];
  read_string_from_file(".beargit/.current_branch", current_branch, BRANCHNAME_SIZE);

  // The first COMMIT_ID_BRANCH_BYTES=10 characters of the commit ID will
  // be used to encode the current branch number. This is necessary to avoid
  // duplicate IDs in different branches, as they can have the same pre-
  // decessor (so next_commit_id has to depend on something else).
  int n = get_branch_number(current_branch);
  for (int i = 0; i < COMMIT_ID_BRANCH_BYTES; i++) {
    commit_id[i] = digits[n%3];
    n /= 3;
  }

  // Use next_commit_id to fill in the rest of the commit ID.
  next_commit_id_part1(commit_id + COMMIT_ID_BRANCH_BYTES);
}


// This helper function returns the branch number for a specific branch, or
// returns -1 if the branch does not exist.
int get_branch_number(const char* branch_name) {
  FILE* fbranches = fopen(".beargit/.branches", "r");

  int branch_index = -1;
  int counter = 0;
  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), fbranches)) {
    strtok(line, "\n");
    if (strcmp(line, branch_name) == 0) {
      branch_index = counter;
    }
    counter++;
  }

  fclose(fbranches);

  return branch_index;
}

/* beargit branch */

int beargit_branch() {

  FILE* fbranches = fopen(".beargit/.branches", "r");

  char branches[FILENAME_SIZE];
  char current_branch[BRANCHNAME_SIZE];
  read_string_from_file(".beargit/.current_branch", current_branch, BRANCHNAME_SIZE);
  while (fgets(branches, sizeof(branches), fbranches)) {
    strtok(branches, "\n");
    if (!strcmp(branches, current_branch)) {
      fprintf(stdout, "* ");
    }
    fprintf(stdout, "%s\n", branches);
  }

  fclose(fbranches);

  return 0;
}

/* beargit checkout */

int checkout_commit(const char* commit_id) {

  // Delete all files specified in current .index
  FILE *findex = fopen(".beargit/.index", "r");
  char line[FILENAME_SIZE];
  while (fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    fs_rm(line);
  }
  fclose(findex);

  // If commit_id == 00.0, empty .index, write commit_id to current .prev
  if (!strcmp(commit_id, "0000000000000000000000000000000000000000")) {
    write_string_to_file(".beargit/.index", "");
    write_string_to_file(".beargit/.prev", commit_id);
    return 0;
  }

  // Copy .index file from commit_id dir to current dir
  char commit_dir[80];
  char index_dir[80];
  sprintf(commit_dir, ".beargit/%s/", commit_id);
  sprintf(index_dir, "%s.index", commit_dir);
  fs_cp(index_dir, ".beargit/.index");

  // Copy all files specified in this .index from commit_id to current dir
  char file_dir[80];
  findex = fopen(".beargit/.index", "r");

  while (fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    strcpy(file_dir, commit_dir);
    fs_cp(strcat(file_dir, line), line);
  }
  fclose(findex);

  // Write commit_id to current .prev
  write_string_to_file(".beargit/.prev", commit_id);
  
  return 0;
}

int is_it_a_commit_id(const char* commit_id) {

  int i, n;
  char c;
  n = strlen(commit_id);
  if (n != 40) {
    return 0;
  } else {
    for (i = 0; i < n; i++) {
      c = commit_id[i];
      if (c != '6' && c != '1' && c != 'c') {
        return 0;
      }
    }
  }
  return 1;
}

int beargit_checkout(const char* arg, int new_branch) {
  // Get the current branch
  char current_branch[BRANCHNAME_SIZE];
  read_string_from_file(".beargit/.current_branch", current_branch, BRANCHNAME_SIZE);

  // If not detached, update the current branch by storing the current HEAD into that branch's file...
  // Even if we cancel later, this is still ok.
  if (strlen(current_branch)) {
    char current_branch_file[BRANCHNAME_SIZE+50];
    sprintf(current_branch_file, ".beargit/.branch_%s", current_branch);
    fs_cp(".beargit/.prev", current_branch_file);
  }

  // Check whether the argument is a commit ID. If yes, we just stay in detached mode
  // without actually having to change into any other branch.
  if (is_it_a_commit_id(arg)) {
    char commit_dir[FILENAME_SIZE] = ".beargit/";
    strcat(commit_dir, arg);
    if (!fs_check_dir_exists(commit_dir)) {
      fprintf(stderr, "ERROR: Commit %s does not exist\n", arg);
      return 1;
    }

    // Set the current branch to none (i.e., detached).
    write_string_to_file(".beargit/.current_branch", "");

    return checkout_commit(arg);
  }

  // Just a better name, since we now know the argument is a branch name.
  const char* branch_name = arg;

  // Read branches file (giving us the HEAD commit id for that branch).
  int branch_exists = (get_branch_number(branch_name) >= 0);

  // Check for errors.
  if (branch_exists && new_branch) {
    fprintf(stderr, "ERROR: A branch named %s already exists\n", branch_name);
    return 1;
  } else if (!branch_exists && !new_branch) {
    fprintf(stderr, "ERROR: No branch %s exists\n", branch_name);
    return 1;
  }

  // File for the branch we are changing into.
  char branch_file[80];
  strcpy(branch_file, ".beargit/.branch_"); 
  strcat(branch_file, branch_name);

  // Update the branch file if new branch is created (now it can't go wrong anymore)
  if (new_branch) {
    FILE* fbranches = fopen(".beargit/.branches", "a");
    fprintf(fbranches, "%s\n", branch_name);
    fclose(fbranches);
    fs_cp(".beargit/.prev", branch_file); 
  }

  write_string_to_file(".beargit/.current_branch", branch_name);

  // Read the head commit ID of this branch.
  char branch_head_commit_id[COMMIT_ID_SIZE];
  read_string_from_file(branch_file, branch_head_commit_id, COMMIT_ID_SIZE);

  // Check out the actual commit.
  return checkout_commit(branch_head_commit_id);
}
