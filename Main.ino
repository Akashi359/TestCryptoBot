#include "MyPreferences.h"
#include "MyWifi.h"
#include "MyHttp.h"

const boolean RESET = false;

void setup()
{
  //setup serial port
  Serial.begin(115200);

  if(RESET){
    struct InfoObject infoObj;
    readInfoObject(&infoObj);
    infoObj.error = false;
    strncpy(infoObj.errorString,"Placeholder - last writer was a reset", MAX_ERRORSTRING_LEN);
    infoObj.lastTrade = SOLD;
    infoObj.lastPrice = 0.1;
    infoObj.tradeStrategy = FULL_BUY;
    infoObj.reservedETH = 0.2;
    infoObj.counter = 0;
    writeInfoObject(&infoObj);
  }
  Serial.println("Setup done");
}

void loop()
{
  /*
   * If RESET or ERROR, spin and do nothing:
   */
  if(RESET){
    Serial.println("Will not run program while RESET is true");
    delay(5000);
    return;
  }
  struct InfoObject infoObj;
  readInfoObject(&infoObj);
  if(infoObj.error){
    Serial.println(infoObj.errorString);
    delay(5000);
    return;
  }
  Serial.println("----------------------------------------");
  Serial.print("counter: ");Serial.println(infoObj.counter++);
  writeInfoObject(&infoObj);

  /*
   * Get wifi connection:
   */
  if(!myWiFiConnect()){
    Serial.println("Could not connect to wifi");
    delay(5000);
    return;
  }
  Serial.println("Got Connection");
  
  /*
   * Get fear and greed index value:
   */
  long fng = getFNG();
  if(fng < 0){
    Serial.println("getFNG() returned < 0");
    delay(5000);
    return;
  }
  else if(fng > 80){
    infoObj.tradeStrategy = NO_BUY;
    writeInfoObject(&infoObj);
  }
  else if(fng > 70) {
    infoObj.tradeStrategy = HALF_BUY;
    writeInfoObject(&infoObj);
  }
  else if(fng < 30 && infoObj.tradeStrategy != FULL_BUY){
    infoObj.lastPrice = 0;
    infoObj.tradeStrategy = FULL_BUY;
    writeInfoObject(&infoObj);
  }
  Serial.print("fng: ");Serial.println(fng);
  if(infoObj.tradeStrategy == NO_BUY){
    Serial.println("Current trade strategy is NO_BUY");
  }
  else if(infoObj.tradeStrategy == HALF_BUY){
    Serial.println("Current trade strategy is HALF_BUY");
  }
  else if(infoObj.tradeStrategy == FULL_BUY){
    Serial.println("Current trade strategy is FULL_BUY");
  }

  /*
   * Get price change:
   */
  short numPlus, numMinus, numTotal;
  if(!getPriceChange(&numPlus, &numMinus, &numTotal)){
    Serial.println("Could not get price change");
    delay(5000);
    return;
  }
  Serial.print("numPlus: ");Serial.print(numPlus);
  Serial.print(" numMinus: ");Serial.print(numMinus);
  Serial.print(" numTotal: ");Serial.println(numTotal);
  
  /*
   * Current ETH Price:
   */
  char ethPriceAsChar[MAX_PRICESTRING_LEN];
  if(!getETHPrice(ethPriceAsChar, MAX_PRICESTRING_LEN)){
    Serial.println("Could not get eth price");
    delay(5000);
    return;
  }
  double ethPriceAsDouble = strtod(ethPriceAsChar, nullptr);
  if(ethPriceAsDouble == 0.0){
    Serial.println("Could not convert eth price to double");
    delay(5000);
    return;
  }
  Serial.print("Current ETH price: "); Serial.println(ethPriceAsChar);
  if(infoObj.lastTrade == SOLD){
    Serial.println("last transaction was a sell");
  }
  else if(infoObj.lastTrade == BOUGHT){
    Serial.println("last transaction was a buy");
  }
  Serial.print("Price during last transaction: "); Serial.println(infoObj.lastPrice);
  
  /*
   * Get current ETH and USD balances:
   */
  char freeUSD[MAX_ASSETSTRING_LEN];
  char freeETH[MAX_ASSETSTRING_LEN];
  if(!getBalances(freeETH, MAX_ASSETSTRING_LEN, freeUSD, MAX_ASSETSTRING_LEN)){
    Serial.println("Could not get balances");
    delay(5000);
    return;
  }
  double freeETHAsDouble = strtod(freeETH, nullptr);
  if(freeETHAsDouble <= 0.0){
    Serial.println("Could not convert free ETH to double");
    delay(5000);
    return;
  }
  if(freeETHAsDouble <= infoObj.reservedETH){
    infoObj.error = true;
    strncpy(infoObj.errorString,"Not enough free ETH to sell.", MAX_ERRORSTRING_LEN);
    writeInfoObject(&infoObj);
    return;
  }
  double freeUSDAsDouble = strtod(freeUSD, nullptr);
  if(freeUSDAsDouble <= 0.0){
    Serial.println("Could not convert free USD to double");
    delay(5000);
    return;
  }
  Serial.print("Current freeETH: "); Serial.println(freeETH);
  Serial.print("Current freeUSD: "); Serial.println(freeUSD);
  
  /*
   * Trade:
   */
  char lastPrice[MAX_PRICESTRING_LEN];
  if(infoObj.lastTrade == BOUGHT && numPlus >= numTotal -2 && ethPriceAsDouble > infoObj.lastPrice){
    char amountToSell[MAX_ASSETSTRING_LEN];
    snprintf(amountToSell, MAX_ASSETSTRING_LEN, "%f", freeETHAsDouble - infoObj.reservedETH);
    if(!sell(amountToSell, infoObj.errorString, lastPrice, MAX_PRICESTRING_LEN)){
      infoObj.error = true;
      writeInfoObject(&infoObj);
      return;
    }
    infoObj.lastTrade = SOLD;
    infoObj.lastPrice = strtod(lastPrice, nullptr);
    if(infoObj.lastPrice == 0.0){
      infoObj.error = true;
      strncpy(infoObj.errorString,"Could not convert lastPrice to double", MAX_ERRORSTRING_LEN);
      writeInfoObject(&infoObj);
      return;
    }
    Serial.print("Made sell. Price: "); Serial.println(lastPrice);
    writeInfoObject(&infoObj);
  }
  else if(infoObj.lastTrade == SOLD && numMinus >= numTotal -2){
    char amountToBuy[MAX_ASSETSTRING_LEN];
    amountToBuy[0] = '\0';
    char lastPrice[MAX_PRICESTRING_LEN];
    if(infoObj.tradeStrategy == FULL_BUY){
      snprintf(amountToBuy, MAX_ASSETSTRING_LEN, freeUSD);
    }
    else if(infoObj.tradeStrategy == HALF_BUY){
      snprintf(amountToBuy, MAX_ASSETSTRING_LEN, "%f", freeUSDAsDouble / 2);
    }
    if(infoObj.tradeStrategy != NO_BUY && amountToBuy[0] != '\0'){
      if(!buy(amountToBuy, infoObj.errorString, lastPrice, MAX_PRICESTRING_LEN)){
        infoObj.error = true;
        writeInfoObject(&infoObj);
        return;
      }
      infoObj.lastTrade = BOUGHT;
      infoObj.lastPrice = strtod(lastPrice, nullptr);
      if(infoObj.lastPrice == 0.0){
        infoObj.error = true;
        strncpy(infoObj.errorString,"Could not convert lastPrice to double", MAX_ERRORSTRING_LEN);
        writeInfoObject(&infoObj);
        return;
      }
      Serial.print("Made buy. Price: "); Serial.println(lastPrice);
      writeInfoObject(&infoObj);
    }
  }

  //15 minutes
  delay(900000);
}
