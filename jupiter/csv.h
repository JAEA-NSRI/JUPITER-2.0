
#ifndef CSV_CSV_H
#define CSV_CSV_H

#include "common.h"
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

enum csv_error_level {
  CSV_EL_DEBUG = 0, ///< Error level for debug
  CSV_EL_INFO,      ///< Error level for info
  CSV_EL_WARN,      ///< Error level for warning
  CSV_EL_ERROR,     ///< Error level for error
  CSV_EL_FATAL,     ///< Error level for fatal

  CSV_EL_MAX
};
typedef enum csv_error_level csv_error_level;

enum csv_error {
  CSV_ERR_SUCC = 0,  ///< No error
  CSV_ERR_EOF,       ///< Unexpected end of file while parsing.
  CSV_ERR_MIX_QUOTE, ///< Quoted and non-quoted strings are mixed.
  CSV_ERR_2BIG,     ///< Too big data
  CSV_ERR_DATA_AFTER_CONT, ///< Data found after continuation mark.
  CSV_ERR_BLOCK_COMMENT_AFTER_CONT, ///< Block comment found after cont.
  CSV_ERR_FOPEN,    ///< Could not open file

  CSV_ERR_NOMEM,    ///< Memory allocation failed

  CSV_ERR_CMDLINE_INVAL, ///< Invalid command line argument
  CSV_ERR_CMDLINE_RANGE, ///< Range error in command line argument

  CSV_ERR_SYS,      ///< System Error
  CSV_ERR_MPI,      ///< MPI Error
  CSV_ERR_GEOMETRY, ///< Geometry Error
  CSV_ERR_SERIALIZE, ///< Serializer Error
};
typedef enum csv_error csv_error;

struct csv_column;
struct csv_row;
struct csv_data;

typedef struct csv_column csv_column;
typedef struct csv_row    csv_row;
typedef struct csv_data   csv_data;

/**
 * @brief Parse CSV from stream and store to data.
 * @param stream Stream that to be parsed.
 * @param data Pointer to be stored.
 * @param error_line number of line as text which an error occured.
 * @papam error_col  number of column as text which an error occured.
 * @return See below
 *
 * This function allocates the data for you.
 * After you use, you must free with *freeCSV*
 * (do not use standard free).
 *
 * @warning Even if an error occured, data will be nullified.
 *
 * Location of error set to error_line and error_col if they are not NULL.
 * If no error occured, error_line and error_col will not be changed.
 *
 * This function clears errno to zero even if there are no error.
 *
 * Return or errno values:
 * ~~~
 * Value         | Description
 * ------------- | ----------------------------------------------------------
 * CSV_ERR_SUCC  | Successful end.
 *               |
 * CSV_ERR_NOMEM | Allocation failed.
 * CSV_ERR_SYS   | Read failed. See man read(2), and details may be in errno.
 *               |
 * CSV_ERR_EOF   | Unexpected end of file.
 * ~~~
 */
JUPITER_DECL
csv_error readCSV(FILE *stream, csv_data **data,
                  long *error_line, long *error_col);

/**
 * @brief Free all regions allocated by readCSV.
 * @param d Allocated pointer to csv_data.
 *
 * This function also free the pointer d itself.
 *
 * This function never fail unless d is invalid pointer.
 * If d is NULL, this function will do nothing.
 */
JUPITER_DECL
void freeCSV(csv_data *d);

/**
 * @brief create new empty CSV data.
 * @return Allocated data
 *
 * After you use, you must free with freeCSV().
 */
JUPITER_DECL
csv_data *allocateCSV(void);

/**
 * @brief Allocate new CSV row data.
 */
JUPITER_DECL
csv_row *newCSVRow(csv_data *d);
JUPITER_DECL
csv_column *newCSVColumn(csv_row *d);

/**
 * @brief append CSV row to data d.
 * @param d Pointer to control data.
 * @param row Pointer to row.
 * @return Pointer to next column of c, NULL if c is NULL or no more next.
 *
 * row must be a pointer which is created by newCSVRow(d).
 * row must not be already inserted to d. If so, it will cause undefined
 * behavior.
 */
JUPITER_DECL
int appendCSVRow(csv_data *d, csv_row *row);

/**
 * @brief append CSV col to row.
 * @param row Pointer to row.
 * @param col Pointer to column.
 * @return Pointer to next column of c, NULL if c is NULL or no more next.
 *
 * row and col must be a pointer which is created for same CSV control data.
 * col must not be already inserted to row. If so, it will cause undefined
 * behavior.
 */
JUPITER_DECL
int appendCSVColumn(csv_row *row, csv_column *col);

/**
 * @brief get the next column of c
 * @param c Pointer to column.
 * @return Pointer to next column of c, NULL if c is NULL or no more next.
 */
JUPITER_DECL
csv_column *getNextColumn(csv_column *c);

/**
 * @brief get the previous column of c
 * @param c Pointer to column.
 * @return Pointer to previous column of c, NULL if c is NULL or first
 */
JUPITER_DECL
csv_column *getPrevColumn(csv_column *c);

/**
 * @brief get the next row of r
 * @param r Pointer to row.
 * @return Pointer to row column of r, NULL if r is NULL or no more next.
 */
JUPITER_DECL
csv_row *getNextRow(csv_row *r);

/**
 * @brief get the previous row of r
 * @param r Pointer to row.
 * @return Pointer to previous row of r, NULL if r is NULL or first.
 *
 * getPrevColumn for the first row will return last row data.
 */
JUPITER_DECL
csv_row *getPrevRow(csv_row *r);

/**
 * @brief Get Row data from CSV data d
 *
 * If index is negative, index of absolute value,
 * from the last item (the last item is -1).
 */
JUPITER_DECL
csv_row *getRowOfCSV(csv_data *d, int index);

/**
 * @brief Get Column data from CSV Row d
 *
 * If index is negative, index of absolute value,
 * from the last item (the last item is -1).
 */
JUPITER_DECL
csv_column *getColumnOfCSV(csv_row *d, int index);

/**
 * @brief Get the value of CSV cell c
 *
 * You must not modify the returned string.
 * You must not free the returned pointer (usually cause invalid free).
 *
 * returned text will always null-terminated.
 */
JUPITER_DECL
const char *getCSVValue(csv_column *c);

/**
 * @brief Set the value of CSV cell c to text.
 * @param c cell pointer to be set.
 * @param text text to be set.
 * @param len length of text (*include* NUL char).
 * @return CSV_ERR_NOMEM if memory allocation failed. CSV_ERR_SUCC if success.
 *
 * This function will allocate some memory and
 * will copy the text to the allocated memory.
 *
 * If text is longer than len, the text will be cut off to
 * length of len.
 */
JUPITER_DECL
csv_error setCSVValue(csv_column *c, const char *text, size_t len);

/**
 * @brief Find the row which the first column is keystr.
 * @param d Pointer to CSV data
 * @param keystr string to search for
 * @param len length of keystr (*include* NUL char)
 * @return Pointer to found row. NULL if not found or invalid argument
 */
JUPITER_DECL
csv_row *findCSVRow(csv_data *d, const char *keystr, int len);

/**
 * @brief Find the next row which the first column is same key value.
 * @param o Pointer to CSV data.
 * @return Pointer to found row. NULL if not found or invalid argument
 */
JUPITER_DECL
csv_row *findCSVRowNext(csv_row *o);

/**
 * @brief Get the line number of loaded file where the cell c is written.
 * @param c Pointer to cell data.
 * @return Line number. 0 or negative if cannot be detemined.
 */
JUPITER_DECL
long getCSVTextLineOrigin(csv_column *c);

/**
 * @brief Get the column number of loaded file where the cell c is written.
 * @param c Pointer to cell data.
 * @return Column number. 0 or negative if cannot be detemined.
 */
JUPITER_DECL
long getCSVTextColumnOrigin(csv_column *c);

/**
 * @brief Set the line/column number of loaded file
 *        where the cell c is written.
 * @param col Pointer to cell data.
 * @param l line number.
 * @param c column number.
 *
 * Sets EINVAL to errno if col is NULL.
 */
JUPITER_DECL
void setCSVTextOrigin(csv_column *col, long l, long c);

/**
 * @brief dump csv data to stream
 *
 * Debug use.
 */
JUPITER_DECL
void dumpCSV(FILE *stream, csv_data *t);

#ifdef __cplusplus
}
#endif

#endif /* CSV_CSV_H */
