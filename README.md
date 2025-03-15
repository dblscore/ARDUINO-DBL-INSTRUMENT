# DBL Instrument - A Real-Time MIDI Interpreter

## Introduction

DBL Instrument is an open-source project implementing a **Degree-Based Language (DBL)**-based musical instrument. This language allows direct manipulation of melodic degrees, independently of fixed pitches and underlying harmonic structures. Inspired by the **Samchillian** and **Misha**, this instrument offers an innovative approach where each key is associated with a degree variation rather than an absolute note.

## Main Features

- **Degree-based navigation**: Move up, down, or arpeggiate within a chord or scale in real time.
- **Harmonic synchronization**: A central controller dynamically transmits scales and chords to instruments.
- **Custom MIDI mapping**: Each key on a MIDI keyboard is mapped to a DBL symbol.
- **Network communication**: Uses **Multicast UDP** for harmonic structure transmission.
- **Harmonic and polyphonic modes**: Play chords in addition to single notes.

## Required Hardware

- **Arduino Leonardo** (or compatible)
- **W5500 Ethernet Shield** (for network communication)
- **MIDI IN/OUT interface** (MIDI Shield or native USB MIDI)
- **Arduino Mega 2560** (for the central controller)
- **Ethernet cables and network switch**

## Installation and Setup

1. **Clone the repository**
   ```sh
   git clone https://github.com/dblscore/ARDUINO-DBL-INSTRUMENT.git
   cd ARDUINO-DBL-INSTRUMENT
   ```
2. **Install Arduino libraries**
   - MIDI Library: [GitHub](https://github.com/FortySevenEffects/arduino_midi_library)
   - Ethernet3 Library: [Arduino Docs](https://docs.arduino.cc/libraries/ethernet/)
   - MIDIUSB Library: [Arduino Docs](https://docs.arduino.cc/libraries/midiusb/)
   - Adafruit GFX and SSD1306 (for OLED display)

3. **Flash the firmware**
   - Upload **instrument.ino** to **Arduino Leonardo**.
   - Upload **controller.ino** to **Arduino Mega 2560**.

## Usage

1. **System Setup**
   - Connect the Arduino boards via USB and Ethernet.
   - Connect MIDI instruments to Arduino Leonardo.
   - Start the central controller (Arduino Mega) to synchronize scales and chords.

2. **Interacting with the Instrument**
   - Use a MIDI keyboard to play with DBL symbols.
   - Adjust harmonic settings via the built-in web interface.

## Example DBL Symbols

| Symbol | Action |
|---------|--------|
| `\` | Decrease degree by one unit |
| `/` | Increase degree by one unit |
| `n` | Move up to the next chord note |
| `p` | Move down to the previous chord note |
| `o` | Move down to the tonic |
| `O` | Move up to the tonic |

## Resources and Documentation

- **Scientific article on DBL**: [JIM 2021](https://www.youtube.com/watch?v=TLWImOY1gJg)
- **Demo video**: [YouTube](https://www.youtube.com/watch?v=SwaCKtd4l5s)
- **DBL website**: [dbl-score.net](https://www.dbl-score.net/)

## Contributions

Contributions are welcome! Potential improvements include:
- Implementing additional DBL symbols
- Enhancing network latency performance
- Designing more intuitive interfaces

To contribute, fork the project and submit a **Pull Request**.

## License

This project is licensed under the **MIT License**.

---

*Created by [Emanuele Di Mauro](mailto:emanuele.dimauro@gmail.com), inspired by research in computational musicology.*

