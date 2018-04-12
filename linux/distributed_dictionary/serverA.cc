/*************************************************
  Filename: serverA.cc
  Creator: Hemajun
  Description: backend server A.
*************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define UDPPORT             21146           // udp port.
#define FILENAME            "backendA.txt"  // data file.
#define MAXLINECOUNT        1024            // max line count.

enum FUNCTION {
    FUNC_SEARCH,
    FUNC_PREFIX
};

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

/*************************************************
  Description： [Helper] parse string to int.
*************************************************/
int str_to_int(const char * str)
{
    int num = 0;
    int len = strlen(str);
    for (int i = 0; i < len; i++) 
        if (str[i] >= '0' and str[i] <= '9')
            num = num * 10 + (str[i] - '0');
    return num;
}

/*************************************************
  Description: substring
*************************************************/
const char * substring(const char * str, const int & start, const int & len)
{
    char * new_str = new char[len + 1];
    for (int i = 0; i < len; i++)
        new_str[i] = str[i + start];
    new_str[len] = '\0';
    return new_str;
}

/*************************************************
  Description: substring
*************************************************/
const char * substring(const char * str, const int & start)
{
    return substring(str, start, strlen(str) - start - 1);
}

/*************************************************
  Description: Determine if str1 is a prefix of str2.
  if prefix, return 1; else reture 0.
*************************************************/
int prefix(const char * str1, const char * str2)
{
    int ret = 0;
    int len_str1 = strlen(str1);
    int len_str2 = strlen(str2);
    if (len_str1 <= len_str2 && !strcmp(str1, substring(str2, 0, len_str1)))
    {
        ret = 1;
    }
    return ret;
}

/*************************************************
  Description： [Business] start server.
  if success, return sockfd; else return -1.
*************************************************/
int start_server(const int & port)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 != sockfd)
    {
        sockaddr_in addr_server;
        bzero(&addr_server, sizeof(sockaddr_in));
        addr_server.sin_family = AF_INET;
        addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
        addr_server.sin_port = htons(port);
        if (-1 != bind(sockfd, (sockaddr *)&addr_server, sizeof(sockaddr_in)))
            return sockfd;
        close(sockfd);
    }
    return -1;
}
/*************************************************
  Description: [Business] get data from file.
*************************************************/
void get_data(const char * input, int * out_matches, char ** out_result)
{
    FILE * fp = fopen(FILENAME, "r");
    if (!fp)
    {
        fprintf(stderr, "Open file failure.\n");
        return;
    }
    int len_input = strlen(input);
    char * key = new char[len_input + 4];
    strcpy(key, input);
    strcpy(key + len_input, " :: ");
    char * buf = new char[MAXLINECOUNT];
    bzero(buf, MAXLINECOUNT);
    while(fgets(buf, MAXLINECOUNT, fp))
    {
        if (prefix(key, buf))
        {
            ++(*out_matches);
            int len = strlen(buf) - (len_input + 4);
            *out_result = new char[len + 2];
            (*out_result)[0] = '<';
            strcpy((*out_result) + 1, buf + len_input + 4);
            strcpy((*out_result) + len, ">\n");
            break;
        }
        bzero(buf, MAXLINECOUNT);
    }
    delete key;
    fclose(fp);
}

/*************************************************
  Description: [Business] process request and send back response.
*************************************************/
void process_request(const int & sockfd)
{
    // recv request
    char ch_cmd;
    sockaddr_in addr_aws;
    socklen_t len_addr = sizeof(sockaddr_in);
    recvfrom(sockfd, &ch_cmd, 1, 0, (sockaddr *)&addr_aws, &len_addr);
    FUNCTION func = FUNCTION(str_to_int(&ch_cmd));
    int len_input = 27;
    char * buf_recv = new char[len_input + 1];
    bzero(buf_recv, len_input);
    int len = recvfrom(sockfd, buf_recv, len_input, 0, (sockaddr *)&addr_aws, &len_addr);
    buf_recv[len] = '\0';
    printf("The ServerA received input <%s> and operation <%s>.\n", buf_recv, func == FUNC_SEARCH ? "search" : "prefix");

    int matches_count = 0;
    char * result = new char();
    get_data(buf_recv, &matches_count, &result);
    delete buf_recv;

    // send response
    char str_func[1];
    sprintf(str_func, "%d", func);
    sendto(sockfd, str_func, 1, 0, (sockaddr *)&addr_aws, len_addr);
    char str_matches_count[8];
    sprintf(str_matches_count, "%8d", matches_count);
    sendto(sockfd, str_matches_count, 8, 0, (sockaddr *)&addr_aws, len_addr);
    int len_result = strlen(result);
    char str_len_result[8];
    sprintf(str_len_result, "%8d", len_result);
    sendto(sockfd, str_len_result, 8, 0, (sockaddr *)&addr_aws, len_addr);
    sendto(sockfd, result, len_result, 0, (sockaddr *)&addr_aws, len_addr);
    printf("The ServerA finished sending the output to AWS.\n");
}

/*************************************************
  Description： [Business] entry.
*************************************************/
int main(int argc, char const *argv[])
{
    // start server
    int sockfd = start_server(UDPPORT);
    if (-1 == sockfd)
    {
        fprintf(stderr, "Boot backend server A failure.\n");
        return -1;
    }
    printf("The ServerA is up and running using UDP on port <%d>.\n", UDPPORT);

    // whole loop
    while(1)
        process_request(sockfd);

    return 0;
}
