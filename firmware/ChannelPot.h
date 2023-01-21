class ChannelPot {
  int mPin;
  int mNumberOfChannels;

  /*

    Split the fader up into 13 parts (0-12), which correpsond with 3 channels.

    0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12
    |    3    |              2            |      1      |
    The channels at the sides of potentiometer need less room since they locate easily by butting up against the end of the fader.

  */
  int mLookupTable[4][12] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2 },
    { 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3 }
  };

public:
  ChannelPot(int pin);
  int selectedChannelIdx();
  void setNumberOfChannels(int numberOfChannels);
};

ChannelPot::ChannelPot(int pin) {
  mPin = pin;
}

int ChannelPot::selectedChannelIdx() {
  int channelInput = analogRead(mPin);
  channelInput = map(channelInput, 0, 4095, 0, 11);
  return mLookupTable[mNumberOfChannels-1][channelInput];
}

void ChannelPot::setNumberOfChannels(int numberOfChannels) {
  mNumberOfChannels = numberOfChannels;
}
