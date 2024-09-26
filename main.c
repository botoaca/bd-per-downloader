#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <sys/stat.h>
#include <curl/curl.h>

typedef struct {
    char* src;
    char* start;
    char* end;
    short verbose;
} args_t;

typedef struct {
    char* response;
    size_t size;
} memory_t;

args_t process_args(int argc, char* argv[]) {
    args_t args = { NULL, NULL };

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--src") == 0 && i + 1 < argc) {
            args.src = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "--start") == 0 && i + 1 < argc) {
            args.start = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "--end") == 0 && i + 1 < argc) {
            args.end = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            args.verbose = 1;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }

    if (args.src == NULL || args.end == NULL) {
        fprintf(stderr, "Invalid arguments\n");
        exit(EXIT_FAILURE);
    }
    if (args.start == NULL) args.start = args.end;

    return args;
}


size_t write_mem_cb(void* data, size_t size, size_t nmemb, void* clientp)
{
    size_t realsize = size * nmemb;
    memory_t* mem = (memory_t*)clientp;

    char* ptr = realloc(mem->response, mem->size + realsize + 1);
    if (!ptr) return 0;

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

void download_pdf(char* url, args_t args) {
    CURL* curl = curl_easy_init();
    char* filepath = malloc(strlen(args.src) + strlen(strrchr(url, '/') + 1) + 2);
    sprintf(filepath, "%s/%s", args.src, strrchr(url, '/') + 1);
    FILE* file = fopen(filepath, "wb");

    if (file == NULL) {
        fprintf(stderr, "Failed to open file\n");
        exit(EXIT_FAILURE);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    CURLcode res = curl_easy_perform(curl);

    if (args.verbose) printf("Downloading from: %s to %s\n", url, filepath);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        exit(EXIT_FAILURE);
    }

    curl_easy_cleanup(curl);
}

memory_t fetch_html(char* url) {
    CURL* curl = curl_easy_init();
    memory_t chunk = { 0 };
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_mem_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        exit(EXIT_FAILURE);
    }

    curl_easy_cleanup(curl);
    return chunk;
}

short is_in_range(const char* start_date, const char* end_date, const char* check_date) {
    short start_year, start_month, end_year, end_month, check_year, check_month;
    sscanf(start_date, "%hd/%hd", &start_year, &start_month);
    sscanf(end_date, "%hd/%hd", &end_year, &end_month);
    sscanf(check_date, "%hd/%hd", &check_year, &check_month);

    if (strcmp(check_date, start_date) == 0 || strcmp(check_date, end_date) == 0) return 1;
    if (check_year < start_year || check_year > end_year) return 0;
    if (check_year == start_year && check_month < start_month) return 0;
    if (check_year == end_year && check_month > end_month) return 0;

    return 1;
}

void parse_html_and_download_pdfs(char* html, args_t args, char* root_url) {
    const char* pattern = "href\\s*=\\s*[\"']([^\"']+\\.pdf)[\"']";
    
    regex_t regex;
    regmatch_t match[2];

    if (regcomp(&regex, pattern, REG_EXTENDED)) {
        printf("Could not compile regex\n");
        exit(EXIT_FAILURE);
    }

    const char* p = html;
    while (regexec(&regex, p, 2, match, 0) == 0) {
        char* pdf_url = malloc(match[1].rm_eo - match[1].rm_so + 1);
        strncpy(pdf_url, p + match[1].rm_so, match[1].rm_eo - match[1].rm_so);
        pdf_url[match[1].rm_eo - match[1].rm_so] = '\0';
        char* pdf_url_copy = malloc(strlen(root_url) + strlen(pdf_url) + 1);
        sprintf(pdf_url_copy, "%s%s", root_url, pdf_url);

        char* tok = strtok(pdf_url, "/");
        char* year = NULL;
        char* month = NULL;
        char* temp = NULL;
        while (tok != NULL) {
            year = month;
            month = temp;
            temp = tok;
            tok = strtok(NULL, "/");
        }

        if (strlen(year) == 5) year = "2009"; // this is solely a Scînteia specific thing,
        //                                       in 2009, the Jurnalul României paper was
        //                                       including issues of Scînteia from 20 years back
        char* date = malloc(strlen(year) + strlen(month) + 2);
        sprintf(date, "%s/%s", year, month);

        if (is_in_range(args.start, args.end, date)) download_pdf(pdf_url_copy, args);

        p += match[0].rm_eo;
    }

    regfree(&regex);
}

int main(int argc, char* argv[]) {
    args_t args = process_args(argc, argv);

    char* root_url = "http://bibliotecadeva.eu:82/periodice/";
    char* publication = args.src;
    char* full_url = malloc(strlen(root_url) + strlen(publication) + strlen(".html") + 1);
    struct stat st = {0};
    sprintf(full_url, "%s%s.html", root_url, publication);

    if (stat(args.src, &st) == -1)
        if (mkdir(args.src, 0700) != 0) {
            fprintf(stderr, "Error creating directory: %s\n", args.src);
            exit(EXIT_FAILURE);
        }

    char* html = fetch_html(full_url).response;
    parse_html_and_download_pdfs(html, args, root_url);

    return 0;
}