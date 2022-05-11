#include "HTTPClient.h"
#include "WiFiUdp.h"
#include "NTPClient.h"
#include "MyPreferences.h"
#include "MyHttpUtilities.h"
#include "MyHttp.h"

const short timeoutLimit = 20; //measured in number of 200 ms increments

const char *apiKey = "put an apikey here";

/*
 * Get the amount of freeETH and freeUSD
 * Returns true on success, false on failure
 */
boolean getBalances(char* freeETH, const size_t freeETHLen, char* freeUSD, const size_t freeUSDLen){
    /*
     * Payload buffer
     * 37 for "https://api.binance.us/api/v3/account"
     * 1 for the "?" character
     * 23 for the query string:
     *     - 10 for "timestamp="
     *     - 10 for the epoch in seconds
     *     - 3 for trailing zeroes to "convert" epoch to ms
     * 1 for & character in url.
     * 74 for the signature:
     *     - 10 for "signature="
     *     - 64 for the 32 byte signature in hex
     * 1 for the null terminator
     */
    const size_t URL_LEN = 37;
    const size_t QUERY_LEN = 23;
    const size_t SIG_LEN = 74;
    char payload[URL_LEN+1+QUERY_LEN+1+SIG_LEN+1];
    char* query = payload + URL_LEN + 1;
    char* signature = query + QUERY_LEN + 1;
     
    /*
     * Create totalParams.
     * In this case, its just a timestamp.
     */
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP);
    timeClient.begin();
    for(short k = 0; !timeClient.update(); k++){
      if(k > timeoutLimit){
        Serial.println("timeClient.update() did not return true within timeoutLimit");
        timeClient.end();
        return false; 
      }
      delay(200);
    }
    sprintf(query, "timestamp=%lu000", timeClient.getEpochTime());
    timeClient.end();

    /*
     * Sign the payload.
     */
    byte byteSignature[32];
    if(!signPayload(byteSignature, query))
      return false;
    //Convert signature to charBuffer
    int prefixLen = sprintf(signature, "signature=");
    for(int k = 0; k < 32; k++){
      snprintf(signature+prefixLen+2*k, 3, "%02X", byteSignature[k]);
    }
    
    /*
     * Make the request
     */
    HTTPClient http;
    //Set Address and query parameters
    //Can't use printf because we don't want to overwrite with null terminators
    memcpy(payload, "https://api.binance.us/api/v3/account", URL_LEN*sizeof(char));
    payload[URL_LEN] = '?';
    payload[URL_LEN + 1 + QUERY_LEN] = '&';
    if(!http.begin(payload)){
      Serial.println("http.begin returned false");
      return false;
    }
    //Set Headers
    http.addHeader("X-MBX-APIKEY", apiKey, true, false);
    //Send request
    int httpCode = http.GET();
    if(httpCode != 200){
      Serial.println("status code not equal to 200");
      return false;
    }

    ResponseParser rspParser;
    if( !rspParser.loadPayload(&http) ||
        !rspParser.getValue("asset\":\"ETH\",\"free", freeETH, freeETHLen) ||
        !rspParser.getValue("asset\":\"USD\",\"free", freeUSD, freeUSDLen) ){
      return false;
    }
    return true;
}

/*
 * Returns true on success.
 * If successful and the price has increased 5%, *numPlusPtr will be incremented by one.
 * If successful and the price has decreased 5%, *numMinusPtr will be incremented by one.
 * If successful and the price has neither increased nor decreased by 5%, neither value will be incremented.
 * If unsuccessful, *numPlusPtr and *numMinusPtr will be unchanged
 */
boolean get24hrChangeSingle(const char* symbol, short* numPlusPtr, short* numMinusPtr){
    /*
     * Payload buffer
     * 41 for "https://api.binance.us/api/v3/ticker/24hr"
     * 1 for the "?" character
     * 17 for the query string:
     *     - 7 for "symbol="
     *     - 10 for the length of the actual symbol
     * 1 for the null terminator
     */
    size_t URL_LEN = 41;
    size_t SYMBOL_KEY_LEN = 7;
    size_t SYMBOL_VAL_LEN = 10;
    char payload[URL_LEN+1+SYMBOL_KEY_LEN+SYMBOL_VAL_LEN+1];
    sprintf(payload, "https://api.binance.us/api/v3/ticker/24hr");
    payload[URL_LEN] = '?';
    sprintf(payload+URL_LEN+1, "symbol=");
    if(snprintf(payload+URL_LEN+1+SYMBOL_KEY_LEN, SYMBOL_VAL_LEN+1, symbol) > SYMBOL_VAL_LEN){
      Serial.println("symbol was too large to fit in preallocated buffer");
      return false;
    }
    
    HTTPClient http;
    if(!http.begin(payload)){
      Serial.println("http.begin returned false");
      return false;
    }
    //Send request
    int httpCode = http.GET();
    if(httpCode != 200){
      Serial.println("status code not equal to 200");
      return false;
    }
    
    /*
     * Process Response
     */
    char priceChangeAsString[64];
    ResponseParser rspParser;
    if( !rspParser.loadPayload(&http) ||
        !rspParser.getValue("priceChangePercent", priceChangeAsString, 64) ){
      return false;
    }

    double priceChangeAsDouble = strtod(priceChangeAsString, nullptr);
    if(priceChangeAsDouble == 0.0)
      return false;
    else if(priceChangeAsDouble >= 5.0){
      (*numPlusPtr)++;
    }
    else if(priceChangeAsDouble <= -5.0){
      (*numMinusPtr)++;
    }
    Serial.print(symbol);Serial.print(":");Serial.print(priceChangeAsString); Serial.print(",");
    return true;
}

boolean getPriceChange(short* numPlusPtr, short* numMinusPtr, short* numTotalPtr){
    *numTotalPtr = 8;
    *numPlusPtr = *numMinusPtr = 0;
    if(!get24hrChangeSingle("ETHUSD", numPlusPtr, numMinusPtr)){Serial.println("ETHUSD"); return false;}
    if(!get24hrChangeSingle("BTCUSD", numPlusPtr, numMinusPtr)){Serial.println("BTCUSD"); return false;}
    if(!get24hrChangeSingle("LINKUSD", numPlusPtr, numMinusPtr)){Serial.println("LINKUSD"); return false;}
    if(!get24hrChangeSingle("VETUSD", numPlusPtr, numMinusPtr)){Serial.println("VETUSD"); return false;}
    if(!get24hrChangeSingle("ONEUSD", numPlusPtr, numMinusPtr)){Serial.println("ONEUSD"); return false;}
    if(!get24hrChangeSingle("MATICUSD", numPlusPtr, numMinusPtr)){Serial.println("MATICUSD"); return false;}
    if(!get24hrChangeSingle("SOLUSD", numPlusPtr, numMinusPtr)){Serial.println("SOLUSD"); return false;}
    //if(!get24hrChangeSingle("VTHOUSD", numPlusPtr, numMinusPtr)){Serial.println("VTHOUSD"); return false;}
    if(!get24hrChangeSingle("ADAUSD", numPlusPtr, numMinusPtr)){Serial.println("ADAUSD"); return false;}
    Serial.println("");

    return true;
}

boolean getETHPrice(char* priceBuffer, const size_t priceBufferLen){
    /*
     * Payload buffer
     */
    char payload[] = "https://api.binance.us/api/v3/ticker/price?symbol=ETHUSD";
    
    HTTPClient http;
    if(!http.begin(payload)){
      Serial.println("http.begin returned false");
      return false;
    }
    //Send request
    int httpCode = http.GET();
    if(httpCode != 200){
      Serial.println("status code not equal to 200");
      return false;
    }
    
    /*
     * Process Response
     */
    ResponseParser rspParser;
    if( !rspParser.loadPayload(&http) ||
        !rspParser.getValue("price", priceBuffer, priceBufferLen) ){
      return false;
    }
    return true;
}

long getFNG(){
    HTTPClient http;
    if(!http.begin("https://api.alternative.me/fng/")){
        Serial.println("http.begin returned false");
        return -1;
    }

    if(http.GET() != 200){
        Serial.println("Request to fear and greed index did not return 200");
        return -1;
    }
    
    /*
     * Process Response
     */
    char fngAsString[32];
    ResponseParser rspParser;
    if( !rspParser.loadPayload(&http) ||
        !rspParser.getValue("value", fngAsString, 32) ){
      return -1;
    }
    long retVal = strtol(fngAsString, nullptr, 10);
    if(retVal == 0L)
      return -1;
    return retVal;
}

/*
 * This class exists so I don't forget to free the payload buffer.
 * After an HTTP response is received and the status has been assessed as 200,
 * Load the ResponseParser with the HTTPClientObject once,
 *  then getValue() to extract json values.
 * Note this isn't a real json parser, it just treats the json object
 *  as a string and searches for a substring.
 */
class TradeMaker{
  private:
    TradeMaker (const TradeMaker&) = delete;
    TradeMaker& operator= (const TradeMaker&) = delete;

    char* reqPayload;

  public:
    TradeMaker();
    ~TradeMaker();
    boolean performTrade(const char* side, const char* amount, char* errorString, char* priceTradedAt, size_t priceTradedAtLen);
};
TradeMaker::TradeMaker(){
  reqPayload = nullptr;
}

TradeMaker::~TradeMaker(){
  free(reqPayload);
}

boolean TradeMaker::performTrade(const char* side, const char* amount, char* errorString, char* priceTradedAt, size_t priceTradedAtLen){
  if(reqPayload != nullptr){
    free(reqPayload);
    reqPayload = nullptr;
  }

  /*
   * Allocate Payload
   */
  char url[] = "https://api.binance.us/api/v3/order";
  /*
   * timestamp and signature must be penultimate and last, respectively
   * Payload buffer
   * 13 for the symbol string:
   *     - "symbol=ETHUSD"
   * 1 for & character in url.
   * 11 for the type string:
   *     - "type=MARKET"
   * 1 for & character in url.
   * 5 for the side key:
   *     - "side="
   * XXX for the side,
   *     - taken as an argument
   * 1 for & character in url.
   * 14 or 9 for amount key length, depending on the side:
   *     - "quoteOrderQty=" (buy) or "quantity=" (sell)
   * XXX for the amount,
   *     - taken as an argument
   * 1 for & character in url.
   * 23 for the timestamp string:
   *     - 10 for "timestamp="
   *     - 10 for the epoch in seconds
   *     - 3 for trailing zeroes to "convert" epoch to ms
   * 1 for & character in url.
   * 74 for the signature:
   *     - 10 for "signature="
   *     - 64 for the 32 byte signature in hex
   * 1 for the null terminator.
   */
  const size_t SIDE_KEY_LEN = 5;
  const size_t SIDE_VAL_LEN = strlen(side);
  const size_t TYPE_LEN = 11;
  const size_t SYMBOL_LEN = 13;
  size_t AMOUNT_KEY_LEN;
  if(strcmp(side, "BUY") == 0){
    AMOUNT_KEY_LEN = 14;
  }
  else if(strcmp(side, "SELL") == 0){
    AMOUNT_KEY_LEN = 9;
  }
  else{
    Serial.println("argument 'side' is neither 'BUY' nor 'SELL'");
    return false;
  }
  const size_t AMOUNT_VAL_LEN = strlen(amount);
  const size_t TIMESTAMP_LEN = 23;
  const size_t SIG_LEN = 74;

  const size_t PAYLOAD_LEN = SYMBOL_LEN +1+ TYPE_LEN +1+ SIDE_KEY_LEN + SIDE_VAL_LEN +1+ AMOUNT_KEY_LEN + AMOUNT_VAL_LEN +1+ TIMESTAMP_LEN +1+ SIG_LEN;
  //PAYLOAD_LEN must be without null terminator, because we use this in the call to http.POST
  //If the null terminator is sent over http, then it counts as part of the signature and the signature check fails.
  char* payload = (char*)malloc(sizeof(char)*(PAYLOAD_LEN+1));
  if(payload == NULL){
    snprintf(errorString, MAX_ERRORSTRING_LEN, "Failed to malloc trade request payload.");
    return false;
  }

  int charsWritten;
  /*
   * Populate payload:
   */
  if(strcmp(side, "BUY") == 0){
    charsWritten = sprintf(payload, "symbol=ETHUSD&type=MARKET&side=%s&%s%s&", side, "quoteOrderQty=", amount);
  }
  else{
    charsWritten = sprintf(payload, "symbol=ETHUSD&type=MARKET&side=%s&%s%s&", side, "quantity=", amount);
  }

  /*
   * Timestamp
   * Don't put the & character in yet, because we need to sign without it.
   */
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP);
  timeClient.begin();
  for(short k = 0; !timeClient.update(); k++){
    if(k > timeoutLimit){
      snprintf(errorString, MAX_ERRORSTRING_LEN, "timeClient.update() did not return true within timeoutLimit");
      timeClient.end();
      return false; 
    }
    delay(200);
  }
  charsWritten += sprintf(payload+charsWritten, "timestamp=%lu000", timeClient.getEpochTime());
  timeClient.end();

  /*
   * Sign the payload.
   */
  //payload+charsWritten is the location of the '&' char between timestamp and signature
  //its currently a null terminator
  //we want to leave it a null terminator because the '&' should not be part of the signature hash
  char* signature = payload + charsWritten + 1;
  byte byteSignature[32];
  if(!signPayload(byteSignature, payload)){
    snprintf(errorString, MAX_ERRORSTRING_LEN, "failed to sign payload");
    return false;
  }
  //Convert signature to charBuffer
  int sigKeyLen = sprintf(signature, "signature=");
  for(int k = 0; k < 32; k++){
    snprintf(signature+sigKeyLen+2*k, 3, "%02X", byteSignature[k]);
  }
  //place the '&' char between the timestamp and signature
  payload[charsWritten] = '&';

  Serial.print("request: ");Serial.println(payload);

  /*
   * Make the request
   */
  HTTPClient http;
  if(!http.begin(url)){
    snprintf(errorString, MAX_ERRORSTRING_LEN, "http begin did not return true");
    return false;
  }
  //Set Headers
  http.addHeader("X-MBX-APIKEY", apiKey, true, false);
  //Send request
  int httpCode = http.POST((uint8_t *)payload, (PAYLOAD_LEN*sizeof(uint8_t))/sizeof(char));
  /*if(httpCode != 200){
    snprintf(errorString, MAX_ERRORSTRING_LEN, "response status did not equal 200");
    return false;
  }*/
  
  /*
   * Verify response
   */
  char rspPayload[32];
  ResponseParser rspParser;
  if( !rspParser.loadPayload(&http) ||
      !rspParser.getValue("status", rspPayload, 32) ){
    snprintf(errorString, MAX_ERRORSTRING_LEN, "no element 'status' in response payload");
    return false;
  }
  if(strcmp(rspPayload, "FILLED") != 0){
    snprintf(errorString, MAX_ERRORSTRING_LEN, "value to key 'status' did not equal 'FILLED'");
    return false;
  }
  if(!rspParser.getValue("fills\":[{\"price", priceTradedAt, priceTradedAtLen)){
    snprintf(errorString, MAX_ERRORSTRING_LEN, "Could not retrieve fill price");
    return false;
  }
  return true;
}

boolean buy(char* quoteOrderQty, char* errorString, char* priceTradedAt, size_t priceTradedAtLen){
  TradeMaker tradeMaker;
  return tradeMaker.performTrade("BUY", quoteOrderQty, errorString, priceTradedAt, priceTradedAtLen);
}

boolean sell(char* quantity, char* errorString, char* priceTradedAt, size_t priceTradedAtLen){
  TradeMaker tradeMaker;
  return tradeMaker.performTrade("SELL", quantity, errorString, priceTradedAt, priceTradedAtLen);
}

/*
 * Possible returns from deserializeJson():
 *     Serial.println(DeserializationError::Ok);//0
 *     Serial.println(DeserializationError::EmptyInput);//1
 *     Serial.println(DeserializationError::IncompleteInput);//2
 *     Serial.println(DeserializationError::InvalidInput);//3
 *     Serial.println(DeserializationError::NoMemory);//4
 *     Serial.println(DeserializationError::TooDeep);//5
 */
//https://randomnerdtutorials.com/esp32-ntp-client-date-time-arduino-ide/
//https://github.com/espressif/arduino-esp32/tree/master/libraries/HTTPClient/src
//HTTPClient uses WiFiClient.h for the stream pointer
  //https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiClient.h
  //WiFiClient.h doesn't have any documentation though. that's available here instead:
  //https://www.arduino.cc/reference/en/language/functions/communication/stream/streamavailable/
      //the readbytes function is kind of messed up, i can't find any function that matches
      //the declaration of int read(uint8_t *buf, size_t size); in WiFiClient.h exactly.
      //readbytes, but returning -1 is the closest.
  //https://www.arduino.cc/reference/en/language/functions/communication/stream/streamreadbytes/
  //https://www.arduino.cc/reference/en/language/functions/communication/stream/streampeek/
  
