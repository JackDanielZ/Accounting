#include <stdio.h>
#include <Elementary.h>

#include "common.h"

#define ERR(fmt, ...) fprintf(stderr, fmt"\n", ## __VA_ARGS__)

static char*
_file_get_as_string(const char *filename)
{
   char *file_data = NULL;
   int file_size;
   FILE *fp = fopen(filename, "r");
   if (!fp)
     {
        ERR("Can not open file: \"%s\".", filename);
        return NULL;
     }

   fseek(fp, 0, SEEK_END);
   file_size = ftell(fp);
   if (file_size == -1)
     {
        fclose(fp);
        ERR("Can not ftell file: \"%s\".", filename);
        return NULL;
     }
   rewind(fp);
   file_data = (char *) calloc(1, file_size + 1);
   if (!file_data)
     {
        fclose(fp);
        ERR("Calloc failed");
        return NULL;
     }
   int res = fread(file_data, file_size, 1, fp);
   fclose(fp);
   if (!res)
     {
        free(file_data);
        file_data = NULL;
        ERR("fread failed");
     }
   return file_data;
}

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   eina_init();

   if (argc <= 1)
     {
        ERR("Usage: %s desc_file", argv[0]);
        exit(1);
     }
   const char *buffer = _file_get_as_string(argv[1]);
   if (!buffer) exit(1);
   desc_parse(buffer);

//   elm_run();
   eina_shutdown();
   return 0;
}
ELM_MAIN()
