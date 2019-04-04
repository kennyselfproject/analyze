#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

typedef struct tagRow
{
    int     cols;
    int     colmax;
    int     nozeros;
    double  total;
    char   *colvalues[0];
}Row;

typedef struct tagTable
{
    int  rows;
    int  rowmax;
    Row *rowvalues[0];
}Table;

char *read_data_from_file(char *filepath, long *size);
long  write_data_to_file(char *filepath, char *data, long size);
char *handle_one_row_in_buffer(
        char *buffer, long size, char **columns, int *colnum);
void  handle_data_in_buffer(
        char *buffer, long size, int filterId, char *filter, char *targetFile);
void  handle_data_by_filter(Table *table, int filterId, char *filter);
Row  *find_row_by_filter(Table *table, int filterId, char *filter);

char *print_row_to_buffer(Row *row, char *cache);
long  print_table_to_buffer(Table *table, char *cache);

#define ROWS    1000000
// #define _DEBUG_

int main(int argc, char *argv[])
{
    char *sourceFile = (char*)"stdin.txt";
    char *targetFile = (char*)"stdout.txt";

    long  fileSize = 0;
    char *buffer = NULL;
    int   filterId = 1;
    char *filter = NULL;

    switch(argc)
    {
    case 5:
        filterId = atoi(argv[4]);
    case 4:
        filter = argv[3];
    case 3:
        targetFile = argv[2];
    case 2:
        sourceFile = argv[1];
    case 1:
        printf("[INFO] the source file [%s], target file [%s]\n", 
               sourceFile, targetFile);
        break;

    default:
        printf("The command syntax as following:\n"
               "\t%s [source_file [target_file]]",
               argv[0]);
        exit(1);
        break;
    }

    buffer = read_data_from_file(sourceFile, &fileSize);
    if (buffer == NULL)
        goto error;

    handle_data_in_buffer(buffer, fileSize, filterId, filter, targetFile);

error:
    if (buffer != NULL)
    {
        free(buffer);
        buffer = NULL;
    }

    return 0;
}

char *handle_one_row_in_buffer(char *buffer, long size, Row *row, int *colnum)
{
    static int head = 1;
    char   *start = buffer;
    char   *cur = buffer;
    char   *end = buffer + size;
    int     num = 0;
    double  total = 0;
    int     zeros = 0;
    
    for (cur = buffer; cur < end; cur++)
    {
        switch(*cur)
        {
        case ' ':
        case '\t':
        case '\0':
            if (row != NULL)
	    {
                *cur = '\0';
		if (head != 1 && num > 1)
		  total += atof(start);
	    }
            start = cur + 1;
            break;

        case '\n':
        case '\r':
            if (row != NULL)
	    {
                *cur = '\0';
		if (head != 1 && num > 1)
		  total += atof(start);
	    }
            goto ret;

        default:
            if (cur == start)
            {
                if (row != NULL)
                {
                    row->colvalues[num] = start;
                }
                if (*start == '0' && (*(start+1) == '\t' || *(start+1) == '\n'))
                    zeros++;
                num++;
            }

            break;
        }
    }

ret:
    if (colnum != NULL)
        *colnum = num;

    if (row != NULL){
        row->nozeros = num - zeros - 2;
        row->cols = num;
	row->total = total;
    }

    return cur < end ? cur + 1 : NULL;
}

char *print_row_to_buffer(Row *row, char *cache)
{
    int    col = 0;
    int    zeros = 0;
    int    totals = 0;
    int    cols = row->cols;
    double total = 0;
    char **colvalues = row->colvalues;

    for (col = 0; col < cols; col++)
    {
        if (colvalues[col] != NULL)
        {
            char *colvalue = colvalues[col];
            sprintf(cache, "%s\t", colvalue);
	    total += atof(colvalue);
            cache += strlen(cache);
            if (*colvalue == '0' && *(colvalue+1) == '\0')
                zeros++;
            totals++;
        }
    }

    if (row->nozeros == -1)
      sprintf(cache, "total\ttotalavg\tlefttotal\tlefttotalavg\n");
    else
      sprintf(cache, "%d\t%f\t%d\t%f\n", row->nozeros, row->total/row->nozeros, 
	      totals - zeros - 2, total/(totals - zeros - 2));
    cache += strlen(cache);

    return cache;
}

long print_table_to_buffer(Table *table, char *cache)
{
    int   row = 0;
    int   rows = table->rows;
    char  *current = cache;
    Row **rowvalues = table->rowvalues;

    for (row = 0; row < rows; row++)
    {
#ifdef _DEBUG_
        printf("[INFO] print row [%d] to cache [%p]\n", row, current);
#endif
        current = print_row_to_buffer(rowvalues[row], current);
    }

    return current - cache;
}

Row *find_row_by_filter(Table *table, int filterId, char *filter)
{
    int   i = 0;
    Row  *row = NULL;
    int   rows = table->rows;
    Row **rowvalues = table->rowvalues;

    for (i = 0; i < rows; i++)
    {
        row = rowvalues[i];

        if (filterId < row->cols)
        {
            if (strcmp(row->colvalues[filterId], filter) == 0)
                break;
        }
    }

    return i < rows ? row : NULL;
}

void empty_table_column(Table *table, int col)
{
    int   i = 0;
    Row  *row = NULL;
    int   rows = table->rows;
    Row **rowvalues = table->rowvalues;

    for (i = 0; i < rows; i++)
    {
        row = rowvalues[i];

        if (col < row->cols)
        {
            row->colvalues[col] = NULL;
        }
    }
}

void handle_data_by_filter(Table *table, int filterId, char *filter)
{
    Row   *row = find_row_by_filter(table, filterId, filter);
    char **colvalues = NULL;
    int    cols = 0;
    int    col = 0;

    if (row == NULL)
    {
        printf("[INFO] there are nothing, filterId [%d] filter [%s]\n", 
               filterId, filter);

        return;
    }

    cols = row->cols;
    colvalues = row->colvalues;
    for (col = 2; col < cols; col++)
    {
        double value = atof(colvalues[col]);
        if (value > 0)
            continue;
#ifdef _DEBUG_
        printf("[INFO] remove column [%d]\n", col);
#endif

        empty_table_column(table, col);
    }
}

void handle_data_in_buffer(
        char *buffer, long size, int filterId, char *filter, char *targetFile)
{
    int   num = 0;
    long  cacheSize = 0;
    char *cache = NULL;
    char *current = buffer;
    char *end = buffer + size;

    printf("[INFO] handle the buffer [%p] start, size [%ld]\n", 
           buffer, size);

    // get the column number
    handle_one_row_in_buffer(buffer, size, (Row*)NULL, &num);

    Table *table = (Table*)malloc(sizeof(Table) + ROWS * sizeof(char*));
    printf("[INFO] malloc table [%p——%p]\n", 
           table, (char*)table + sizeof(Table) + ROWS * sizeof(char*));
    table->rows = 0;
    table->rowmax = ROWS;
    
    do
    {
        Row *row = (Row*)malloc(sizeof(Row) + num * sizeof(char*));
#ifdef _DEBUG_
        printf("[INFO] malloc row [%p——%p]\n", 
            row, (char*)row + sizeof(Row) + num * sizeof(char*));
#endif

        row->cols = 0;
        row->colmax = num;
        current = handle_one_row_in_buffer(
                current, size - (current - buffer), row, NULL);
	if (table->rows == 0)
  	    row->nozeros = -1;
        table->rowvalues[table->rows] = row;
        table->rows++;
#ifdef _DEBUG_
        printf("[INFO] current row is: %d\n", table->rows);
#endif
    }while(current != NULL);

    printf("[INFO] handle the buffer [%p] succeed, size [%ld], rows [%d], columns[%d]\n", 
           buffer, size, table->rows, num);

    handle_data_by_filter(table, filterId, filter);

    cache = (char*)malloc(size + table->rows * 20);
    printf("[INFO] malloc cache [%p——%p]\n", cache, (char*)cache + table->rows * 20);
    cacheSize = print_table_to_buffer(table, cache);

    write_data_to_file(targetFile, cache, cacheSize);
}

char *read_data_from_file(char *filepath, long *size)
{
    FILE *stream = NULL;
    long  fileSize = 0;
    char *buffer = NULL;

    if (size != NULL)
        *size = 0;

    printf("[INFO] open file [%s]\n", filepath);
    if((stream = fopen(filepath,"r")) == NULL)
    {
        printf("[ERROR] open file [%s] fail, errno:%d\n", 
               filepath, errno);
        goto error;
    }

    printf("[INFO] get file [%s] size\n", filepath);
    if (fseek(stream, 0, SEEK_END))
    {
        printf("[ERROR] file [%s] fseek end with error [%d]\n", filepath, errno);
        goto error;
    }
    fileSize = ftell(stream);
    if (fseek(stream, 0, SEEK_SET))
    {
        printf("[ERROR] file [%s] fseek head with error [%d]\n", filepath, errno);
        goto error;
    }
    printf("[INFO] the file [%s] size [%ld]\n", filepath, fileSize);

    printf("[INFO] malloc memory buffer size [%ld]\n", fileSize);
    buffer = (char*)malloc(fileSize + 10);
    printf("[INFO] the memory buffer [%p——%p]\n", buffer, buffer + fileSize);

    printf("[INFO] read data from file [%s] size [%ld]\n", 
           filepath, fileSize);
    if (fread(buffer, fileSize, 1, stream) != 1)
    {
        printf("[INFO] read file [%s] with error[%d]\n", 
               filepath, errno);
        goto error;
    }

    printf("[INFO] read file [%s] is closed\n", filepath);
    fclose(stream);
    if (size != NULL)
        *size = fileSize;

    return buffer;

error:
    if (buffer != NULL)
    {
        free(buffer);
        buffer = NULL;
    }

    if (stream != NULL)
    {
        printf("[INFO] read file [%s] is closed\n", filepath);
        fclose(stream);
    }

    return NULL;
}

long  write_data_to_file(char *filepath, char *data, long size)
{
    FILE *stream = NULL;

    printf("[INFO] open file [%s] to write\n", filepath);
    if((stream = fopen(filepath,"w")) == NULL)
    {
        printf("[ERROR] open file [%s] to write fail, errno:%d\n", 
               filepath, errno);
        return -1;
    }

    printf("[INFO] write data [%p] to file [%s] size [%ld]\n", 
           data, filepath, size);
    if (fwrite(data, size, 1, stream) != 1)
    {
        printf("[INFO] write file [%s] with error[%d]\n", 
               filepath, errno);
        goto error;
    }

    printf("[INFO] write file [%s] is closed\n", filepath);
    fclose(stream);

    return size;

error:
    if (stream != NULL)
    {
        printf("[INFO] write file [%s] is closed\n", filepath);
        fclose(stream);
    }

    return 0;
}
