#include "Preferences.h"
#include "MyPreferences.h"
#include "nvs_flash.h"

const char* NAMESPACENAME = "namespace1";

void readInfoObject(struct InfoObject* infoObjPtr){
  Preferences preferences;
  preferences.begin(NAMESPACENAME, true);
  size_t byteCount;

  infoObjPtr->error = preferences.getBool("error", true);
  byteCount = preferences.getBytes("errorString", NULL, NULL);
  preferences.getBytes("errorString", infoObjPtr->errorString, byteCount);
  infoObjPtr->errorString[byteCount/sizeof(char)] = '\0';
  infoObjPtr->lastTrade = preferences.getShort("lastTrade", -1);
  infoObjPtr->lastPrice = preferences.getDouble("lastPrice", -1);
  infoObjPtr->tradeStrategy = preferences.getShort("tradeStrategy", -1);
  infoObjPtr->reservedETH = preferences.getDouble("reservedETH", -1);
  infoObjPtr->counter = preferences.getInt("counter", -1);
  //byteCount = preferences.getBytes("freeETH", NULL, NULL);
  //preferences.getBytes("freeETH", infoObjPtr->freeETH, byteCount);
  //infoObjPtr->freeETH[byteCount/sizeof(char)] = '\0';
  //byteCount = preferences.getBytes("freeUSD", NULL, NULL);
  //preferences.getBytes("freeUSD", infoObjPtr->freeUSD, byteCount);
  //infoObjPtr->freeUSD[byteCount/sizeof(char)] = '\0';

  preferences.end();
}

void writeInfoObject(struct InfoObject* infoObjPtr){
  Preferences preferences;
  preferences.begin(NAMESPACENAME, false);

  preferences.putBool("error", infoObjPtr->error);
  preferences.putBytes("errorString", infoObjPtr->errorString, strlen(infoObjPtr->errorString)*sizeof(char));
  preferences.putShort("lastTrade", infoObjPtr->lastTrade);
  preferences.putDouble("lastPrice", infoObjPtr->lastPrice);
  preferences.putShort("tradeStrategy", infoObjPtr->tradeStrategy);
  preferences.putDouble("reservedETH", infoObjPtr->reservedETH);
  preferences.putInt("counter", infoObjPtr->counter);
  //preferences.putBytes("freeETH", infoObjPtr->freeETH, strlen(infoObjPtr->freeETH)*sizeof(char));
  //preferences.putBytes("freeUSD", infoObjPtr->freeUSD, strlen(infoObjPtr->freeUSD)*sizeof(char));

  preferences.end();
}
//https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/

//open the namespace
//preferences.begin("namespace1", false); //second argument is "is read only"

//write a value
//String putThis = "this is a value";
//preferences.putString("key1", "this is a value");

//remove a key
//preferences.remove("key1");

//Removes all preferences from the namespace
//preferences.clear();

//retrieve a value. Returns 2nd argument if the preference has been cleared
//String printThis = preferences.getString("key1", "key not found");//2nd argument is the return if the key is not found
//Serial.println(printThis);

//close the namespace
//preferences.end();

//Delete the namespace by erasing and re-initting the partition
//nvs_flash_erase(); // erase the NVS partition and...
//nvs_flash_init(); // initialize the NVS partition.

//https://stackoverflow.com/questions/58719387/writing-and-reading-object-into-esp32-flash-memory-arduino
//I think "getBytes(someKey, NULL, NULL)" returns the size of the buffer in bytes
//And then "getBytes(someKey, aBuffer, maxSize)" populates "aBuffer" with at most "maxSize" characters.
