#include "Arduino.h";
#include "HTTPClient.h"

/*
 * Creates an HMAC SHA256 signature for the indicated payload.
 * Returns true on success, false on failure.
 * 
 * Wrapper function that handles allocation and free'ing.
 */
boolean signPayload(byte* signature, const char* payload);

/*
 * This class exists so I don't forget to free the rspBuffer.
 * After an HTTP response is received and the status has been assessed as 200,
 * Load the ResponseParser with the HTTPClientObject once,
 *  then getValue() to extract json values.
 * Note this isn't a real json parser, it just treats the json object
 *  as a string and searches for a substring.
 */
class ResponseParser{
  private:
    ResponseParser (const ResponseParser&) = delete;
    ResponseParser& operator= (const ResponseParser&) = delete;


    char* rspBuffer;
    char* rspBufferEnd;

  public:
    ResponseParser();
    ~ResponseParser();
    boolean loadPayload(HTTPClient*);
    boolean getValue(char* prefix, char* dest, const size_t destLen);
};
