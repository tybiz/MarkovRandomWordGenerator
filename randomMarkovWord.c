#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define EXPORT EMSCRIPTEN_KEEPALIVE
#else
#include <curl/curl.h>
#define EXPORT
#endif

static int table[26][26][27];
bool trained = false;

#ifndef __EMSCRIPTEN__
struct Response
{
  char *data;
  size_t size;
};

static size_t write_callback(void *ptr, size_t size, size_t nmemb, struct Response *r)
{
  size_t total = size * nmemb;
  r->data = realloc(r->data, r->size + total + 1);
  memcpy(r->data + r->size, ptr, total);
  r->size += total;
  r->data[r->size] = '\0';
  return total;
}

char **fetch_words(const char *theme, int *out_count)
{
  char url[512];
  snprintf(url, sizeof(url),
      "https://api.datamuse.com/words?ml=%s&max=200", theme);

  struct Response r = {malloc(1), 0};

  CURL *curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r);
  curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  static char *words[512];
  int count = 0;
  char *p = r.data;
  while ((p = strstr(p, "\"word\":\"")) != NULL)
  {
    p += 8;
    char *end = strchr(p, '"');
    if (!end) break;
    *end = '\0';
    words[count++] = p;
    p = end + 1;
    if (count >= 512) break;
  }

  *out_count = count;
  return words;
}
#endif

EXPORT
void train(const char **words, int count)
{
  for (int i = 0; i < count; i++)
  {
    int len = strlen(words[i]) - 1;
    for (int j = 0; j < len; j++)
    {
      if (!isalpha(words[i][j]) || !isalpha(words[i][j+1])) continue;
      int c0 = tolower(words[i][j]) - 'a';
      int c1 = tolower(words[i][j+1]) - 'a';
      int next = (j+2 < len && isalpha(words[i][j+2]))
                 ? tolower(words[i][j+2]) - 'a'
                 : 26;
      table[c0][c1][next]++;
    }
  }
  trained = true;
}

static int pick_next_letter(int c0, int c1)
{
  int total = 0;
  for (int i = 0; i < 27; i++)
    total += table[c0][c1][i];

  if (total == 0) return -1;

  int r = rand() % total;
  for (int i = 0; i < 27; i++)
  {
    r -= table[c0][c1][i];
    if (r < 0)
      return i;
  }
  return -1;
}

static char output[256];

EXPORT
char* generate_word()
{
  if (!trained)
  {
    printf("Generating before training!");
    return " ";
  }

  int c0, c1, tries = 0;
  do {
    c0 = rand() % 26;
    c1 = rand() % 26;
    int sum = 0;
    for (int i = 0; i < 27; i++) sum += table[c0][c1][i];
    if (sum > 0) break;
  } while (++tries < 1000);

  output[0] = 'a' + c0;
  output[1] = 'a' + c1;
  int len = 2;

  while (len < 16)
  {
    int next_letter = pick_next_letter(c0, c1);
    if (next_letter == -1 || next_letter == 26) break;
    output[len++] = next_letter + 'a';
    c0 = c1;
    c1 = next_letter;
  }

  output[0] = toupper(output[0]);
  output[len] = '\0';
  return output;
}

void printUsage(char *name)
{
  printf("Usage: %s <theme>\n", name);
}

#ifndef __EMSCRIPTEN__
int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printUsage(argv[0]);
    return -1;
  }

  char *theme = argv[1];
  srand(time(NULL));

  int word_count;
  char **words = fetch_words(theme, &word_count);
  printf("Fetched %d words\n", word_count);

  train((const char **)words, word_count);

  char *word = generate_word();
  printf("%s\n", word);

  return 0;
}
#endif

