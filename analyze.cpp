#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

typedef struct tagColumn
{
    int     id;
    double  value;
    char   *strvalue;
}Column;

typedef struct tagRow
{
    int     totalcols;
    int     maxcols;

    /* record the total information */
    int     curcols;
    int     validcols;
    double  sumXn;
    double  sumXXn;
    double  sumXYn;
    double  spearman;
    double  pearson;

    /* record the minimal information */
    int     mcurcols;
    int     mvalidcols;
    double  msumXn;
    double  msumXXn;
    double  msumXYn;
    double  mspearman;
    double  mpearson;

    /* record the column information*/
    Column  columns[0];
}Row;

typedef struct tagTable
{
    int  totalrows;
    int  maxrows;
    int  totalcols;
    int  leftcols;
    Row *rowvalues[0];
}Table;

char *read_data_from_file(char *filepath, long *size);
long  write_data_to_file(char *filepath, char *data, long size);
char *handle_one_row_in_buffer(
        char *buffer, long size, char **columns, int *totalcols);
void  handle_data_in_buffer(
        char *buffer, long size, int filterId, char *filter, char *targetFile);
void  handle_data_by_filter(Table *table, Row *filterrow);
Row  *find_row_by_filter(Table *table, int filterId, char *filter);

int compare_column( const void *arg1, const void *arg2 ) ;
void  calc_one_row_spearman(Row *filterrow, Row *row, bool isfull);
void  calc_the_spearman(Table *table, Row *filterrow, bool isfull);

void  calc_one_row_pearson(Row *filterrow, Row *row, bool isfull);
void  calc_the_pearson(Table *table, Row *filterrow, bool isfull);

char *print_row_to_buffer(Row *row, char *cache);
long  print_table_to_buffer(Table *table, char *cache);

#define MAX_ROWS    1000000

#define CALC_LXY(sumXYn, sumXn, sumYn, n) ((sumXYn) - ((sumXn) * (sumYn) / (n)))
#define CALC_LXX(sumXXn, sumXn, n) CALC_LXY((sumXXn), (sumXn), (sumXn), n)

#define CALC_POWER(x) ((x) * (x))
#define CALC_N_NN_1(n) ((n) * (CALC_POWER(n) - 1))
#define CALC_SPEARMAN(d, n)   (1 - ((6*(d))/CALC_N_NN_1(n)))

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
        printf("[INFO] the source file [%s], target file [%s], filter [%s], filterId [%d]\n", 
               sourceFile, targetFile, filter, filterId);
        break;

    default:
        printf("The command syntax as following:\n"
               "\t%s [source_file [target_file [filter [filterId]]]]",
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

char *handle_one_row_in_buffer(char *buffer, long size, Row *row, int *totalcols)
{
    static int head = 1;
    char   *start = buffer;
    char   *cur = buffer;
    char   *end = buffer + size;
    int     colnum = 0;
    int     zeros = 0;
    double  sumXn = 0;
    double  sumXXn = 0;

#ifdef _DEBUG
    printf("[DEBUG] +++++++++++++++++++++++++++++++++++++++++++++++\n");
#endif

    for (cur = buffer; cur < end; cur++)
    {
        switch(*cur)
        {
        case ' ':
        case '\t':
        case '\0':
        case '\n':
        case '\r':
        {
            char old = *cur;
            if (row != NULL)
	    {
                *cur = '\0';
                row->columns[colnum].id = colnum - 1;
                row->columns[colnum].value = 0;
                row->columns[colnum].strvalue = start;

                if (*start == '0' && (*(start+1) == '\0'))
                    zeros++;
                else
                {
                    if (head == 0 && colnum > 1)
                    {
                        double value = atof(start);
                        sumXn += value;
                        sumXXn += value * value;
                        row->columns[colnum].value = value;
#ifdef _DEBUG
                        printf("[DEBUG] sum:%f\tvalue:%f\n", sumXn, value);
#endif
                    }
                }
	    }
            colnum++;
            if (old == '\n' || old == '\r')
            {
                cur++;
                goto ret;
            }
            else
                start = cur + 1;
        }
        break;


        default:
            break;
        }
    }

ret:
    if (totalcols != NULL)
        *totalcols = colnum;

    if (row != NULL){
        row->totalcols = colnum;

        row->curcols = colnum - 2;
        row->validcols = colnum - zeros - 2;
	row->sumXn = sumXn;
	row->sumXXn = sumXXn;
	row->sumXYn = 0;
	row->spearman = 0;
	row->pearson = 0;

        row->mcurcols = colnum - 2;
        row->mvalidcols = colnum - zeros - 2;
	row->msumXn = sumXn;
	row->msumXXn = sumXXn;
	row->msumXYn = 0;
	row->mspearman = 0;
	row->mpearson = 0;

        if (head != 0)
        {
            head = 0;
            row->curcols = -1;
        }
    }

    return cur < end ? cur : NULL;
}

char *print_row_to_buffer(Row *row, char *cache)
{
    int     col = 0;
    int     curcols = row->curcols;
    int     validcols = row->validcols;
    int     mcurcols = row->mcurcols;
    int     mvalidcols = row->mvalidcols;
    Column *columns = row->columns;

    for (col = 0; col < mcurcols + 2; col++)
    {
        if (columns[col].strvalue != NULL)
        {
            char *colvalue = columns[col].strvalue;
            sprintf(cache, "%s\t", colvalue);
            cache += strlen(cache);
        }
    }

    if (row->curcols == -1)
        sprintf(cache, "total\tsumXn\tsumXXn\tavgall\tavgvalid\tspearman\tpearson\t"
                "mtotal\tmsumXn\tmsumXXn\tmavgall\tmavgvalid\tmspearman\tmpearson\n");
    else
    {
        double avgall = curcols <= 0 ? 0 : (row->sumXn / curcols);
        double avgvalid = validcols <= 0 ? 0 : (row->sumXn / validcols);
        double mavgall = mcurcols <= 0 ? 0 : (row->msumXn / mcurcols);
        double mavgvalid = mvalidcols <= 0 ? 0 : (row->msumXn / mvalidcols);
        sprintf(cache, "%d\t%f\t%f\t%f\t%f\t%f\t%f\t%d\t%f\t%f\t%f\t%f\t%f\t%f\n", 
                curcols, row->sumXn, row->sumXXn, avgall, avgvalid,
                row->spearman, row->pearson,
                mcurcols, row->msumXn, row->msumXXn, mavgall, mavgvalid,
                row->mspearman, row->mpearson);
    }
    cache += strlen(cache);

    return cache;
}

long print_table_to_buffer(Table *table, char *cache)
{
    int   row = 0;
    int   totalrows = table->totalrows;
    char  *current = cache;
    Row **rowvalues = table->rowvalues;

    for (row = 0; row < totalrows; row++)
    {
#ifdef _DEBUG
        printf("[INFO] print row [%d] to cache [%p]\n", row, current);
#endif
        current = print_row_to_buffer(rowvalues[row], current);
    }

    return current - cache;
}

int compare_column( const void *arg1, const void *arg2 ) 
{ 
    Column *column1 = (Column*)arg1;
    Column *column2 = (Column*)arg2;

    if (column1->value > column2->value)
        return 1;
    else if (column1->value < column2->value)
        return -1;
    else 
        return 0;
}

void  calc_one_row_spearman(Row *filterrow, Row *row, bool isfull)
{
    int     col = 0;
    int     curcols = isfull ? filterrow->curcols : filterrow->mcurcols;
    double  d = 0;
    Column *filtercolumns = filterrow->columns;
    Column *columns = row->columns;

    if (filterrow->curcols != row->curcols || 
        filterrow->mcurcols != filterrow->mcurcols)
        return;

    for (col = 2; col < curcols + 2; col++)
    {
        d += CALC_POWER(columns[col].id - filtercolumns[col].id);
    }

    if (isfull)
    {
        row->spearman = CALC_SPEARMAN(d, curcols);
#ifdef _DEBUG
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("d [%f] curcols [%d] spearman [%f]\n", d, curcols, row->spearman);
        printf("---------------------------------------------------\n");
#endif

    }
    else
    {
        row->mspearman = CALC_SPEARMAN(d, curcols);
#ifdef _DEBUG
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("d [%f] mcurcols [%d] mspearman [%f]\n", d, curcols, row->mspearman);
        printf("---------------------------------------------------\n");
#endif
    }
}

void  calc_the_spearman(Table *table, Row *filterrow, bool isfull)
{
    Row    *oldrow = NULL;
    Row    *newrow = NULL;
    int     rowlen = 0;
    int     totalrows = 0;
    int     row = 1;
    int     curcols = isfull ? filterrow->curcols : filterrow->mcurcols;

    if (filterrow == NULL)
    {
        printf("[INFO] there are nothing to do by spearman, filterrow is null\n");

        return;
    }

    rowlen = sizeof(Row) + filterrow->maxcols * sizeof(Column);
    oldrow = (Row*)malloc(rowlen);
    newrow = (Row*)malloc(rowlen);

    memcpy(oldrow, filterrow, rowlen);
    qsort(oldrow->columns + 2, curcols, sizeof(Column), compare_column);

    totalrows = table->totalrows;
    for (row = 1; row < totalrows; row++)
    {
        Row *tmprow = table->rowvalues[row];
        memcpy(newrow, tmprow, rowlen);
        qsort(newrow->columns + 2, curcols, sizeof(Column), compare_column);
        calc_one_row_spearman(oldrow, newrow, isfull);
        if (isfull)
            tmprow->spearman = newrow->spearman;
        else
            tmprow->mspearman = newrow->mspearman;
    }
}

void  calc_one_row_pearson(Row *filterrow, Row *row, bool isfull)
{
    double Lxx = 0;
    double Lyy = 0;
    double Lxy = 0;
    
    int     col = 0;
    int     curcols = isfull ? filterrow->curcols : filterrow->mcurcols;
    int     totalcols = filterrow->totalcols;
    double  sumXYn = 0;
    Column *filtercolumns = filterrow->columns;
    Column *columns = row->columns;

    if (filterrow->curcols != row->curcols || 
        filterrow->mcurcols != filterrow->mcurcols)
        return;

    for (col = 2; col < totalcols; col++)
    {
        sumXYn += columns[col].value * filtercolumns[col].value;
    }

    if (isfull)
    {
        row->sumXYn = sumXYn;

        Lxx = CALC_LXX(filterrow->sumXXn, filterrow->sumXn, curcols);
        Lyy = CALC_LXX(row->sumXXn, row->sumXn, curcols);
        Lxy = CALC_LXY(row->sumXYn, filterrow->sumXn, row->sumXn, curcols);

        if (Lxx == 0 || Lyy == 0)
            row->pearson = 0;
        else
            row->pearson = (Lxy * Lxy)/(Lxx*Lyy);

#ifdef _DEBUG
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("sumXn [%f] sumXXn[%f] curcols [%d]\n", filterrow->sumXn, filterrow->sumXXn, curcols);
        printf("sumYn [%f] sumYYn[%f] curcols [%d]\n", row->sumXn, row->sumXXn, curcols);

        printf("sumXYn[%f] lxx [%f] lyy [%f] lxy [%f]\n", row->sumXYn, Lxx, Lyy, Lxy);
        printf("---------------------------------------------------\n");
#endif
    }
    else
    {
        row->msumXYn = sumXYn;

        Lxx = CALC_LXX(filterrow->msumXXn, filterrow->msumXn, curcols);
        Lyy = CALC_LXX(row->msumXXn, row->msumXn, curcols);
        Lxy = CALC_LXY(row->msumXYn, filterrow->msumXn, row->msumXn, curcols);

        if (Lxx == 0 || Lyy == 0)
            row->mpearson = 0;
        else
            row->mpearson = (Lxy * Lxy)/(Lxx*Lyy);

#ifdef _DEBUG
        printf("****************************************************\n");
        printf("msumXn [%f] msumXXn[%f] mcurcols [%d]\n", filterrow->msumXn, filterrow->msumXXn, curcols);
        printf("msumYn [%f] msumYYn[%f] mcurcols [%d]\n", row->msumXn, row->msumXXn, curcols);

        printf("msumXYn[%f] lxx [%f] lyy [%f] lxy [%f]\n", row->msumXYn, Lxx, Lyy, Lxy);
        printf("---------------------------------------------------\n");
#endif
    }
}

void  calc_the_pearson(Table *table, Row *filterrow, bool isfull)
{
    int     totalrows = 0;
    int     row = 1;

    if (filterrow == NULL)
    {
        printf("[INFO] there are nothing to do by pearson, filterrow is null\n");

        return;
    }

    totalrows = table->totalrows;
    for (row = 1; row < totalrows; row++)
    {
        calc_one_row_pearson(filterrow, table->rowvalues[row], isfull);
    }
}

Row *find_row_by_filter(Table *table, int filterId, char *filter)
{
    int   i = 0;
    Row  *row = NULL;
    int   totalrows = table->totalrows;
    Row **rowvalues = table->rowvalues;

    if (filter == NULL)
        return NULL;

    for (i = 0; i < totalrows; i++)
    {
        Row *tmprow = rowvalues[i];
        if (filterId < tmprow->totalcols)
        {
            Column *column = tmprow->columns + filterId;
            if (strcmp(column->strvalue, filter) == 0)
            {
                int rowlen = sizeof(Row) + tmprow->maxcols * sizeof(Column);
                row = (Row*)malloc(rowlen);
                memcpy(row, tmprow, rowlen);
                break;
            }
        }
    }

    return row;
}

void filter_columns_of_one_row(Row *row, Row *filterrow)
{
    Column *filtercolumns = filterrow->columns;
    Column *columns = row->columns;
    int     totalcols = filterrow->totalcols;
    int     col = 0;
    int     curcol = 2;
    int     validcols = 0;

    for (col = 2; col < totalcols; col++)
    {
        if (filtercolumns[col].value > 0)
        {
            memcpy(columns + curcol, columns + col, sizeof(Column));
            columns[curcol].id = curcol - 1;
            if (columns[curcol].value > 0)
                validcols++;
            curcol++;
        }
        else
        {
            double value = columns[col].value;

            row->msumXn -= value;
            row->msumXXn -= value * value;
        }
    }

    row->mcurcols = curcol - 2;
    row->mvalidcols = validcols;
    memset(columns + curcol, 0, (totalcols - curcol) * sizeof(Column));
}

void handle_data_by_filter(Table *table, Row *filterrow)
{
    int     row = 0;
    int     totalrows = 0;

    if (filterrow == NULL)
    {
        printf("[INFO] there are nothing caused by filterrow is null");

        return;
    }

    table->leftcols = filterrow->mvalidcols;

    totalrows = table->totalrows;
    for (row = 0; row < totalrows; row++)
    {
        filter_columns_of_one_row(table->rowvalues[row], filterrow);
    }

    filter_columns_of_one_row(filterrow, filterrow);
}

void handle_data_in_buffer(
        char *buffer, long size, int filterId, char *filter, char *targetFile)
{
    int   maxcols = 0;
    int   rowlen = 0;
    long  cacheSize = 0;
    char *cache = NULL;
    char *current = buffer;
    char *end = buffer + size;
    Row  *filterrow = NULL;

    printf("[INFO] handle the buffer [%p] start, size [%ld]\n", 
           buffer, size);

    // get the column number
    handle_one_row_in_buffer(buffer, size, (Row*)NULL, &maxcols);

    Table *table = (Table*)malloc(sizeof(Table) + MAX_ROWS * sizeof(Row*));
    printf("[INFO] malloc table [%p——%p]\n", 
           table, (char*)table + sizeof(Table) + MAX_ROWS * sizeof(Row*));
    table->totalcols = maxcols;
    table->leftcols = 0;
    table->totalrows = 0;
    table->maxrows = MAX_ROWS;
    rowlen = sizeof(Row) + maxcols * sizeof(Column);
    
    do
    {
        Row *row = (Row*)malloc(rowlen);
#ifdef _DEBUG
        printf("[INFO] malloc row [%p——%p]\n", row, (char*)row + rowlen);
#endif

        row->totalcols = 0;
        row->maxcols = maxcols;
        current = handle_one_row_in_buffer(
                current, size - (current - buffer), row, NULL);
        table->rowvalues[table->totalrows] = row;
        table->totalrows++;
#ifdef _DEBUG
        printf("[INFO] current row is: %d\n", table->totalrows);
#endif
    }while(current != NULL);

    printf("[INFO] handle the buffer [%p] succeed, size [%ld], totalrows [%d], columns[%d]\n", 
           buffer, size, table->totalrows, maxcols);

    filterrow = find_row_by_filter(table, filterId, filter);

    calc_the_pearson(table, filterrow, true);

    calc_the_spearman(table, filterrow, true);

    handle_data_by_filter(table, filterrow);

    // filterrow have been changed, flush the data
    filterrow = find_row_by_filter(table, filterId, filter);

    calc_the_pearson(table, filterrow, false);

    calc_the_spearman(table, filterrow, false);

    cache = (char*)malloc(size + table->totalrows * 200);
    printf("[INFO] malloc cache [%p——%p]\n", cache, (char*)cache + table->totalrows * 20);
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
