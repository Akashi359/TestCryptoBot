#include "mbedtls/md.h"
#include "MyHttpUtilities.h"

//Binance example secret key:
const char *secretKey = "NhqPtmdSJYdKjVHjA7PZj4Mge3R5YNiP1e3UZjInClVN65XAbvqqM6A7H5fATj0j";

const size_t secretKeyLength = strlen(secretKey);

//https://techtutorialsx.com/2018/01/25/esp32-arduino-applying-the-hmac-sha-256-mechanism/

/*
 * Helper function that signs the payload, but doesn't allocate or free any structures
 */
boolean signPayloadHelper(byte* signature, const char* payload, mbedtls_md_context_t *ctxPtr){
    //setup objects
    int mbedRet = mbedtls_md_setup(ctxPtr, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1); //third arg is HMAC(1) or not(0)
    if(mbedRet == MBEDTLS_ERR_MD_BAD_INPUT_DATA){
      Serial.println("mbed bad input data on setup");
      return false;
    }
    else if(mbedRet == MBEDTLS_ERR_MD_ALLOC_FAILED){
      Serial.println("mbed allocation failure");
      return false;
    }

    //Set secret key
    mbedtls_md_hmac_starts(ctxPtr, (const unsigned char*)secretKey, secretKeyLength);

    //Set payload to hash
    mbedtls_md_hmac_update(ctxPtr, (const unsigned char*) payload, strlen(payload));

    //Create signature
    mbedRet = mbedtls_md_hmac_finish(ctxPtr, signature);
    if(mbedRet == MBEDTLS_ERR_MD_BAD_INPUT_DATA){
      Serial.println("mbed bad input data on finish");
      return false;
    }

    return true;
}

/*
 * Creates an HMAC SHA256 signature for the indicated payload.
 * Returns true on success, false on failure.
 * 
 * Wrapper function that handles allocation and free'ing.
 */
boolean signPayload(byte* signature, const char* payload){
    //create some context objects
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);

    //wrap in helper because we want to free ctx regardless of whether
    //the signature succeeds or not
    boolean retVal = signPayloadHelper(signature, payload, &ctx);
    
    //free structures
    mbedtls_md_free(&ctx);

    return retVal;
}


/*
 * Skips whitespace, returns pointer to next non-whitespace character,
 *  or null if end of buffer
 */
char* skipWhitespace(char* startPtr, char* endPtr){
  char* currPtr = startPtr;
  while(currPtr < endPtr){
    if( *currPtr == ' ' ||
        *currPtr == '\t'||
        *currPtr == '\n' ||
        *currPtr == '\r' ||
        *currPtr == '\f' ||
        *currPtr == '\v')
    {
      currPtr++;
    }
    else{
      return currPtr;
    }
  }
  return nullptr;
}

ResponseParser::ResponseParser(){
  rspBuffer = nullptr;
}

ResponseParser::~ResponseParser(){
  free(rspBuffer);
}

boolean ResponseParser::loadPayload(HTTPClient* httpPtr){
  if(rspBuffer != nullptr){
    free(rspBuffer);
    rspBuffer = nullptr;
  }

  WiFiClient* streamPtr = httpPtr->getStreamPtr();
  if(streamPtr == nullptr){
    Serial.println("failed to get streamPtr");
    return false;
  }

  int available = streamPtr->available();
  //available is already measured in bytes.
  //+1 for null terminator
  rspBuffer = (char*)malloc(available+sizeof(char));
  if(rspBuffer == nullptr){
    Serial.println("failed to malloc rspBuffer");
    return false;
  }
  rspBuffer[available/sizeof(char)] = '\0';

  if(streamPtr->read((uint8_t*)rspBuffer, available) < 0){
    Serial.println("unable to read streamPtr");
    free(rspBuffer);
    rspBuffer = nullptr;
    return false;
  }

  rspBufferEnd = rspBuffer + available/sizeof(char);
  //Serial.println(rspBuffer);
  return true;
}

boolean ResponseParser::getValue(char* prefix, char* dest, const size_t destLen){
  if(rspBuffer == nullptr){
    Serial.println("Must loadPayload() before getValue()");
    return false;
  }

  /*
   * Find the start of the key:
   */
  char* keyStart = strstr(rspBuffer, prefix);
  if(keyStart == nullptr){
    Serial.print("Could not find key ");Serial.print(prefix);Serial.println(" in rspPayload");
    return false;
  }

  /*
   * Find and skip the next double quote, colon, and double quote.
   * The value starts at the character right after that.
   */
  char* nextChar = skipWhitespace(keyStart + strlen(prefix), rspBufferEnd);
  if(nextChar == nullptr || *nextChar != '"'){
    Serial.println("Start of value is passed end of rspPayload");
    return false;
  }
  nextChar = skipWhitespace(++nextChar, rspBufferEnd);
  if(nextChar == nullptr || *nextChar != ':'){
    Serial.println("Start of value is passed end of rspPayload");
    return false;
  }
  nextChar = skipWhitespace(++nextChar, rspBufferEnd);
  if(nextChar == nullptr || *nextChar != '"'){
    Serial.println("Start of value is passed end of rspPayload");
    return false;
  }
  char* valStart = nextChar + 1;
  if(valStart >= rspBufferEnd){
    Serial.println("Start of value is passed end of rspPayload");
    return false;
  }

  /*
   * We now have a pointer to the start of the value we are looking for.
   */
  char* valEnd = strstr(valStart, "\"");
  if(valEnd == nullptr){
    Serial.println("Could not find end of value in rspPayload");
    return false;
  }
  if(valEnd-valStart+1 > destLen){
    Serial.println("buffer not large enough to hold value");
    return false;
  }
  snprintf(dest, valEnd-valStart+1, "%s", valStart);
  //Serial.print("dest: "); Serial.println(dest);
  return true;
}
