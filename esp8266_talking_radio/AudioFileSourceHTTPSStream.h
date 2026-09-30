#pragma once

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include <AudioFileSource.h>

// ESP8266Audio's standard HTTP source uses a plain WiFiClient. GitHub Pages
// redirects to HTTPS, so this source uses BearSSL for public HTTPS MP3s.
class AudioFileSourceHTTPSStream : public AudioFileSource {
 public:
  AudioFileSourceHTTPSStream();
  explicit AudioFileSourceHTTPSStream(const char *url);
  ~AudioFileSourceHTTPSStream() override;

  bool open(const char *url) override;
  uint32_t read(void *data, uint32_t len) override;
  uint32_t readNonBlock(void *data, uint32_t len) override;
  bool seek(int32_t pos, int dir) override;
  bool close() override;
  bool isOpen() override;
  uint32_t getSize() override;
  uint32_t getPos() override;

 private:
  uint32_t readInternal(void *data, uint32_t len, bool nonBlocking);

  BearSSL::WiFiClientSecure client_;
  HTTPClient http_;
  int32_t position_ = 0;
  int32_t size_ = -1;
};

