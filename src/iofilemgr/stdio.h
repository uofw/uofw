/**
 * @author artart78
 * @version 6.60
 *
 * The StdioForUser/Kernel libraries
 */

/** @defgroup Stdio StdioForUser/Kernel libraries
 *
 * @{
 */

/** Get the stdin file descriptor.
 *
 * @return The stdin file descriptor.
 */
int sceKernelStdin();

/** Get the stdout file descriptor.
 *
 * @return The stdout file descriptor.
 */
int sceKernelStdout();

/** Get the stderr file descriptor.
 *
 * @return The stderr file descriptor.
 */
int sceKernelStderr();

/** Reopen stdout as a different file.
 *
 * @param file The file name.
 * @param flags The flags passed to sceIoReopen.
 * @param mode The mode passed to sceIoReopen.
 *
 * @return 0 on success, less than 0 otherwise.
 */
int sceKernelStdoutReopen(const char *file, int flags, SceMode mode);

/** Reopen stderr as a different file.
 *
 * @param file The file name.
 * @param flags The flags passed to sceIoReopen.
 * @param mode The mode passed to sceIoReopen.
 *
 * @return 0 on success, less than 0 otherwise.
 */
int sceKernelStderrReopen(const char *file, int flags, SceMode mode);

/** Reset the stdout fd.
 *
 * @return 0 on success, less than 0 otherwise.
 */
int sceKernelStdoutReset();

/** Reset the stderr fd.
 *
 * @return 0 on success, less than 0 otherwise.
 */
int sceKernelStderrReset();

/** Print a formatted string into an opened file.
 *
 * @param fd The file descriptor.
 * @param fmt The string format.
 *
 * @return The number of characters printed on success, less than 0 otherwise.
 */
int fdprintf(int fd, const char *fmt, ...);

/** Print a formatted string to stdout.
 *
 * @param fmt The string format.
 *
 * @return The number of characters printed on success, less than 0 otherwise.
 */
int printf(const char *fmt, ...);

/** Output a character to an opened file.
 *
 * @param c The character to output.
 * @param fd The file descriptor.
 *
 * @return The printed character.
 */
int fdputc(int c, int fd);

/** Get the next character (after the file position indicator) from an opened file.
 *
 * @param fd The file descriptor.
 *
 * @return The read character on success, -1 otherwise.
 */
int fdgetc(int fd);

/** Get the next line from an opened file.
 *
 * @param s The string buffer.
 * @param fd The file descriptor.
 *
 * @return The string on success, NULL otherwise.
 */
char *fdgets(char *s, int fd);

/** Output a character to stdout.
 *
 * @param c The character to output.
 *
 * @return The printed character.
 */
int putchar(int c);

/** Output a string to an opened file.
 *
 * @param s The string to output.
 * @param fd The file descriptor.
 *
 * @return 0.
 */
int fdputs(const char *s, int fd);

/** Output a string to stdout.
 *
 * @param s The string to output.
 *
 * @return 0.
 */
int puts(const char *s);

/** Get the next character from stdin.
 *
 * @return The read character on success, -1 otherwise.
 */
int getchar(void);

/** Get the next line from stdin.
 *
 * @param s The string buffer.
 *
 * @return The string on success, NULL otherwise.
 */
char *gets(char *s);

/** Useless. Do not use.
 *
 * @return Less than zero.
 */
int sceKernelStdioRead();

/** Useless. Do not use.
 *
 * @return Less than zero.
 */
int sceKernelStdioLseek();

/** Useless. Do not use. */
void sceKernelStdioSendChar();

/** Useless. Do not use.
 *
 * @return Less than zero.
 */
int sceKernelStdioWrite();

/** Useless. Do not use.
 *
 * @return Less than zero.
 */
int sceKernelStdioClose();

/** Useless. Do not use.
 *
 * @return Less than zero.
 */
int sceKernelStdioOpen();

/** Register a stdout pipe.
 *
 * @param id The message pipe id.
 *
 * @return 0 on success, less than 0 otherwise.
 */
int sceKernelRegisterStdoutPipe(SceUID id);

/** Register a stderr pipe.
 *
 * @param id The message pipe id.
 *
 * @return 0 on success, less than 0 otherwise.
 */
int sceKernelRegisterStderrPipe(SceUID id);

/** @} */

