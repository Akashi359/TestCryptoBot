/*
 * Does nothing if already connected.
 * Otherwise, attempst to open a new connection.
 * Returns true on success, false on failure.
 * If failed, prints reason to serial.
 */
boolean myWiFiConnect();

/*
 * Disconnects from wifi.
 */
void myWiFiDisconnect();
