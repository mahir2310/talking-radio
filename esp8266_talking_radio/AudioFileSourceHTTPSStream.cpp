#include "AudioFileSourceHTTPSStream.h"

AudioFileSourceHTTPSStream::AudioFileSourceHTTPSStream() {
  client_.setInsecure();
  // Smaller TLS buffers preserve heap for audio decoding.
  client_.setBufferSizes(1024, 512);
  client_.setTimeout(12000);
}

AudioFileSourceHTTPSStream::AudioFileSourceHTTPSStream(const char *url)
    : AudioFileSourceHTTPSStream() {
  open(url);
}

AudioFileSourceHTTPSStream::~AudioFileSourceHTTPSStream() { close(); }

bool AudioFileSourceHTTPSStream::open(const char *url) {
  close();
  position_ = 0;
  size_ = -1;
  opened_ = false;

  http_.setTimeout(12000);
  http_.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http_.useHTTP10(true);
  if (!http_.begin(client_, url)) return false;

  int status = http_.GET();
  Serial.printf("Audio HTTP status: %d, size: %d bytes, free heap: %u\n",
    status, http_.getSize(), ESP.getFreeHeap());
  if (status != HTTP_CODE_OK) {
    http_.end();
    return false;
  }


  size_ = http_.getSize();
  opened_ = true;
  return true;
}

uint32_t AudioFileSourceHTTPSStream::read(void *data, uint32_t len) {
  return readInternal(data, len, false);
}

uint32_t AudioFileSourceHTTPSStream::readNonBlock(void *data, uint32_t len) {
  return readInternal(data, len, true);
}

uint32_t AudioFileSourceHTTPSStream::readInternal(void *data, uint32_t len,
                                                  bool nonBlocking) {
  if (data == nullptr || !opened_) return 0;
  if (size_ >= 0 && position_ >= size_) return 0;

  WiFiClient *stream = http_.getStreamPtr();
  if (stream == nullptr) return 0;
  if (size_ >= 0) {
    uint32_t remaining = static_cast<uint32_t>(size_ - position_);
    if (len > remaining) len = remaining;
  }

  if (!nonBlocking) {
    uint32_t started = millis();
    while (stream->available() < static_cast<int>(len) &&
           static_cast<uint32_t>(millis() - started) < 500) {
      yield();
    }
  }

  size_t available = stream->available();
  if (available == 0) return 0;
  if (len > available) len = available;

  int amountRead = stream->read(reinterpret_cast<uint8_t *>(data), len);
  if (amountRead <= 0) return 0;
  position_ += amountRead;
  return static_cast<uint32_t>(amountRead);
}

bool AudioFileSourceHTTPSStream::seek(int32_t, int) { return false; }

bool AudioFileSourceHTTPSStream::close() {
  http_.end();
  opened_ = false;
  return true;
}

bool AudioFileSourceHTTPSStream::isOpen() { return opened_; }

uint32_t AudioFileSourceHTTPSStream::getSize() {
  return size_ < 0 ? 0 : static_cast<uint32_t>(size_);
}

uint32_t AudioFileSourceHTTPSStream::getPos() {
  return static_cast<uint32_t>(position_);
}

