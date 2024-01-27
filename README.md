# EwiSynth LV2 Plugin - EWI Synthesizer

## Overview

The EwiSynth LV2 Plugin is a specialized synthesizer designed as a sound generator for MIDI breath controllers, such as the AKAI EWI (Electronic Wind Instrument).

## Features

### 1. Harmonizer

- **Harmonizer**: Harmonize your current note with voicings stored internally by sending pitchbend down.

### 2. Rotator

- **Voicing Rotator**: Rotate voicings dynamically for tonal variety and expressiveness.
- **Mode Selection**: Choose between different rotation modes like linear and random.

### 3. Arpeggiator

- **Arpeggiator Control**: Activate the arpeggiator feature for automatic note sequencing.
- **Range Adjustment**: Set the arpeggiator range to control the span of generated notes.
- **Timing Configuration**: Adjust arpeggiator speed with the `ARPTIME` parameter.

### 4. Glissando

- **Auto Glissando**: Adjust glissando speed with the SlewTime parameter.

## MIDI Breath Controller Integration

1. **Breath Control Mapping**: Connect the MIDI breath controller (e.g., AKAI EWI) to the EwiSynth plugin.
2. **Expressive Voicing**: Leverage the Rotator feature for dynamic voicing changes while playing.
3. **Arpeggiated Sequences**: Activate the Arpeggiator for automatic note sequences from the internal voicings.
4. **Glissando Effects**: Use the SlewTime to control the glissando time between notes.

## Installation

1. type `make`
2. Copy the generated `ewisynth.lv2` directory to your LV2 plugin directory (`~/.lv2`).
3. Load the EwiSynth plugin in your LV2-compatible host and configure MIDI input from the breath controller.

## Example Host Integration

1. Load the EwiSynth LV2 Plugin in your LV2-compatible host.
2. Configure MIDI input to receive signals from the AKAI EWI or other breath controllers.
3. Assign plugin parameters to MIDI CC or host automation.

## Tips for Expressive Performance

- Experiment with different Rotator modes to find voicing variations that suit your musical style.
- Activate the Arpeggiator for intricate and automated melodic sequences.
- Use pitchbend up on the breath controller for expressive glissando effects, use pitchbend down to trigger polyphonic voicings.

Enhance your musical performance with the EwiSynth LV2 Plugin, tailored for MIDI breath controllers like the AKAI EWI. Enjoy the expressive features and dynamic control for a unique playing experience.
(readme written by chatGPT :)
