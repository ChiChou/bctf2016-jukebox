#include <stdio.h>
#include <sqlite3.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define _FORTIFY_SOURCE 2

void handler();
int login();

void menu();
void register_menu();

void list();
void pick();
void search_by_title();
void search_by_artist();
void search();
void add();
void delete();
void empty();
void quit();

char name[8];
void *options[10];
unsigned long token;
sqlite3 *db;

int main(int argc, char *argv[])
{
  signal(SIGALRM, (sig_t)handler);
  alarm(30);
  setbuf(stdout, 0);
  int rc = sqlite3_open("data/music.db", &db);
  if (rc) {
    puts("Unable to load database\n");
    exit(-1);
  }
  if (!login())
    exit(-1);

  register_menu();
  menu();
  sqlite3_close(db);
  return 0;
}

void register_menu()
{
  memset(options, 0, sizeof(options));
  options[1] = list;
  options[2] = pick;
  options[3] = search_by_title;
  options[4] = search_by_artist;

  if (token)
  {
    options[5] = add;
    options[6] = delete;
    options[7] = empty;
    options[8] = login;
  }
  options[9] = quit;
}

static int auth_callback(void *data, int argc, char **argv, char **column_names)
{
  return atoi(argv[0]) != 1;
}

int login()
{
  unsigned long hash = 11235;
  int i, len;
  char sql[256];
  char password[16];

  puts("What's your name?");
  if (scanf("%7s", name) != 1)
    exit(-1);

  getchar(); // remove tailing \n

  if (strcmp(name, "root") == 0)
  {
    puts("Password:");
    if (scanf("%8s", password) == 1)
    {
      for (i = 0, len = strlen(password); i < len; i++)
      {
        hash = 33 * hash + (unsigned long) password[i];
      }
      snprintf(sql, sizeof(sql), "select count(*) cnt from users where username = 'root' and password = %ld", hash);
      if (!sqlite3_exec(db, sql, auth_callback, NULL, NULL))
        return (token = 1);
    }
    
    puts("Auth failed.");
    exit(-1);
  }
  return 1;
}

void handler()
{
  puts("timeout\n");
  exit(0);
}

void menu()
{
  int choice;
  void(*func)(void);

  while(1) {
    printf("Welcome to jukebox, %s\n", name);
    puts("1) List all songs");
    puts("2) Pick a song");
    puts("3) Search by title");
    puts("4) Search by artist");

    if (token) {
      puts("5) Add a song to the library");
      puts("6) Delete a song from library");
      puts("7) Empty music library");
    }
    
    puts("8) Logoff");
    puts("9) Quit");
    puts("Your choice:");

    if (scanf("%1d", &choice) != 1 || !choice > 0)
      exit(-1);
    
    func = options[choice];
    if (func) {
      func();
    } else {
      puts("Invalid choice!");
    }
    puts("-------------------------");
  }
}

void quit()
{
  puts("Bye!");
  exit(0);
}

int callback_play(void *data, int argc, char **argv, char **column_names)
{
  int i;
  for (i=0; i < argc; i++)
  {
    printf("%s:\n%s\n\n", column_names[i], argv[i]);
  }
  return 0;
}

void pick()
{
  int choice;
  char *template = "select * from library where id=%d";
  size_t len = strlen(template) + 4;
  char *sql = malloc(len);
  puts("Id of the song:");
  if (scanf("%3d", &choice) != 1)
    exit(-1);

  snprintf(sql, len, template, choice);
  int rc = sqlite3_exec(db, sql, callback_play, NULL, NULL);
  free(sql);
  if (rc) exit(rc);
}

static int search_callback(void *data, int argc, char **argv, char **column_names)
{
  if (argc == 3)
    printf("%s) %s - %s\n", argv[0], argv[1], argv[2]);
  else if (token)
    puts("Error: Something went wrong.");
  else
    exit(-1);
    
  return 0;
}

void search(const char *field)
{
  char keyword[64];
  char *sql;
  char *template = "select id, artist, title from library where %s like '%%%s%%';";
  size_t len = strlen(template);

  puts("Search in music library: ");
  if (scanf("%63s", keyword) != 1)
    exit(-1);

  len += strlen(keyword);
  sql = malloc(len);
  snprintf(sql, len, template, field, keyword);
  puts("Matches:");
  char *msg;
  int rc = sqlite3_exec(db, sql, search_callback, NULL, &msg);
  free(sql);
  if (rc) {
    if (token)
      puts(msg);
    exit(-1);
  }
}

void search_by_title()
{
  search("title");
}

void search_by_artist()
{
  search("artist");
}

void empty()
{
  char choice;
  puts("Are you sure you want to empty the database?");
  if (scanf("%c", &choice) && (choice | 32) == 'y')
    sqlite3_exec(db, "delete from library;", NULL, NULL, NULL);
}

void add()
{
  char title[32];
  char artist[32];
  char lyric[256];

  puts("Title:");
  if (scanf("%31s", title) != 1)
    exit(-1);

  puts("Artist:");
  if (scanf("%32s", artist) != 1)
    exit(-1);

  puts("Lyric:");
  if (scanf("%255s", lyric) != 1)
    exit(-1);

  char template[] = "insert into library(title, artist, lyric) values ('%s', '%s', '%s')";
  char *sql = malloc(sizeof(template) + sizeof(title) + sizeof(artist) + sizeof(lyric));
  sqlite3_exec(db, sql, NULL, NULL, NULL);
  free(sql);
}

void delete()
{
  int id;
  puts("Which to delete?");
  if (scanf("%d", &id) != 1)
    exit(-1);
  
  char sql[255];
  snprintf(sql, sizeof(sql), "delete from library where id=%d;", id);
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  puts(rc ? "Failed" : "Deleted.");
}

void list()
{
  char *sql = "select id, artist, title from library;";
  int rc = sqlite3_exec(db, sql, search_callback, NULL, NULL);
  if (rc)
    exit(rc);
}
