'use strict';

const programs = [
  { name: 'Sequencer + drums', description: 'Three original lo-fi grooves plus three Yamaha-rim variations, each one bar. Turn Filter to move from kick only to the full kit. The fourth button cycles patterns.',
    pots: ['Speed', 'Pattern', 'Variation', 'MIDI note length', 'Drum volume', 'Filter', 'Unused', 'Unused'],
    buttons: ['Next program', 'Direct MIDI', 'Freeze sequence', 'Next pattern'],
    led: 'LED 3 · direct MIDI / LED 4 · sequence locked / LED 5 · Yamaha rim / LED 6 · MIDI clock / LED 9 · step & priority countdown' },
  { name: 'KarplusResonator', description: 'Twelve resonating strings. The saw + EQ source can excite the audio input.',
    pots: ['Volume', 'Excitation', 'Decay', 'Brightness', 'Transpose', 'Dry / wet', 'Detune', 'Width'],
    buttons: ['Next program', 'Note set', 'Sustain', 'Clear'],
    led: 'LEDs 3–9 · input meter' },
  { name: 'JUNO', description: 'Six voices, warm oscillator layers and stereo chorus. Enable audio, raise Volume, and play the keyboard below.',
    pots: ['Volume', 'Cutoff', 'Resonance', 'Env depth', 'Attack', 'Decay', 'Sustain', 'Release'],
    buttons: ['Next program', 'Waveform', 'Chorus', 'Panic'],
    led: 'LEDs 0–5 · active synth voices' }
];
const knobs = [], buttons = [], leds = [];
let online = false, program = -1, sending = Promise.resolve();
const pendingPots = new Map();
const pressed = new Map();

function connection(isOnline) {
  online = isOnline;
  document.body.classList.toggle('offline', !online);
  document.querySelector('#connection-dot').classList.toggle('online', online);
  document.querySelector('#connection').textContent = online ? 'Local simulation · running' : 'Simulator disconnected';
  for (const control of [...buttons.map(b => b.button), ...knobs.map(k => k.slider)]) control.disabled = !online;
  for (const key of noteButtons.values()) key.disabled = !online || program !== 2;
  for (const knob of knobs) knob.dial.setAttribute('aria-disabled', String(!online));
}
function sendControl(command) {
  sending = sending.then(async () => {
    const response = await fetch('/api/control', {
      method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(command),
      signal: AbortSignal.timeout(2000)
    });
    if (!response.ok) throw new Error((await response.json()).error || 'Control update failed');
  }).catch(error => {
    document.querySelector('#error').hidden = false;
    document.querySelector('#error').textContent = `Could not update Pepper: ${error.message}. Check that the local simulator is running.`;
    connection(false);
  });
  return sending;
}
function paintKnob(index, value) {
  const k = knobs[index];
  k.value = value;
  k.dial.style.setProperty('--sweep', `${value * 270}deg`);
  k.dial.style.setProperty('--angle', `${value * 270 - 135}deg`);
  k.dial.setAttribute('aria-valuenow', Math.round(value * 100));
  k.dial.setAttribute('aria-valuetext', `${Math.round(value * 100)} percent`);
  k.slider.value = value;
  k.output.textContent = `${Math.round(value * 100)}%`;
}
function editKnob(index, value) {
  if (!online) return;
  value = Math.max(0, Math.min(1, value));
  knobs[index].editedAt = performance.now();
  paintKnob(index, value);
  pendingPots.set(index, value);
}
setInterval(() => {
  for (const [index, value] of pendingPots) sendControl({ type: 'pot', index, value });
  pendingPots.clear();
}, 35);

for (let i = 0; i < 10; i++) {
  const led = document.createElement('span');
  led.className = 'led'; led.title = `LED ${i}`;
  document.querySelector('#leds').append(led); leds.push(led);
}
for (const side of ['inputs', 'outputs']) {
  for (let i = 0; i < 8; i++) {
    const row = document.createElement('div'); row.className = 'cv-row';
    row.innerHTML = `<i class="jack" title="CV ${side === 'inputs' ? 'input' : 'output'} ${i} — not connected"></i><span>${i}</span>`;
    document.querySelector(`#cv-${side}`).append(row);
  }
}
for (let i = 0; i < 8; i++) {
  const pot = document.createElement('div'); pot.className = 'pot';
  pot.innerHTML = `<div class="dial" role="slider" tabindex="0" aria-valuemin="0" aria-valuemax="100"><div class="knob"></div></div><div class="pot-label"><b>${i}</b><span></span></div><input type="range" min="0" max="1" step="0.005"><output></output>`;
  document.querySelector('#knobs').append(pot);
  const dial = pot.querySelector('.dial'), slider = pot.querySelector('input');
  knobs.push({ dial, slider, output: pot.querySelector('output'), label: pot.querySelector('.pot-label span'), value: .5, editedAt: -Infinity, dragging: false });
  paintKnob(i, i === 0 || i > 5 ? 0 : i === 5 ? 1 : .5);
  slider.addEventListener('input', () => editKnob(i, Number(slider.value)));
  let startY = 0, startValue = 0;
  dial.addEventListener('pointerdown', event => {
    if (!online || event.button !== 0) return;
    event.preventDefault(); dial.focus(); dial.setPointerCapture(event.pointerId);
    knobs[i].dragging = true; startY = event.clientY; startValue = knobs[i].value;
  });
  dial.addEventListener('pointermove', event => {
    if (knobs[i].dragging) editKnob(i, startValue + (startY - event.clientY) / (event.shiftKey ? 1200 : 240));
  });
  for (const event of ['pointerup', 'pointercancel', 'lostpointercapture']) dial.addEventListener(event, () => { knobs[i].dragging = false; });
  dial.addEventListener('keydown', event => {
    const amount = event.shiftKey ? .005 : .02;
    const values = { ArrowUp: knobs[i].value + amount, ArrowRight: knobs[i].value + amount,
      ArrowDown: knobs[i].value - amount, ArrowLeft: knobs[i].value - amount, Home: 0, End: 1 };
    if (Object.hasOwn(values, event.key)) { event.preventDefault(); editKnob(i, values[event.key]); }
  });
}
function press(index) {
  if (!online || pressed.has(index)) return;
  pressed.set(index, performance.now());
  buttons[index].button.classList.add('pressed');
  sendControl({ type: 'button', index, pressed: true });
}
function release(index) {
  if (!pressed.has(index)) return;
  const started = pressed.get(index);
  // Even a very quick tap must survive the device's 5 ms debounce.
  setTimeout(() => {
    if (pressed.get(index) !== started) return;
    pressed.delete(index);
    sendControl({ type: 'button', index, pressed: false });
    buttons[index].button.classList.remove('pressed');
  }, Math.max(0, 45 - (performance.now() - started)));
}
for (let i = 0; i < 4; i++) {
  const wrap = document.createElement('div'); wrap.className = 'button-wrap';
  wrap.innerHTML = `<button type="button">${i}</button><small></small>`;
  document.querySelector('#buttons').append(wrap);
  const button = wrap.querySelector('button'); buttons.push({ button, label: wrap.querySelector('small') });
  button.addEventListener('pointerdown', event => {
    if (event.button !== 0) return;
    button.setPointerCapture(event.pointerId); press(i);
  });
  for (const event of ['pointerup', 'pointercancel', 'lostpointercapture']) button.addEventListener(event, () => release(i));
  button.addEventListener('keydown', event => {
    if (event.key === ' ' || event.key === 'Enter') { event.preventDefault(); if (!event.repeat) press(i); }
  });
  button.addEventListener('keyup', event => {
    if (event.key === ' ' || event.key === 'Enter') { event.preventDefault(); release(i); }
  });
  button.addEventListener('blur', () => release(i));
  // Support activation by assistive technology without pointer/key events.
  button.addEventListener('click', event => { if (event.detail === 0 && !pressed.has(i)) { press(i); release(i); } });
}
function releaseAll() { for (const i of pressed.keys()) release(i); releaseNotes(); }
window.addEventListener('blur', releaseAll);
document.addEventListener('visibilitychange', () => { if (document.hidden) releaseAll(); });

function showProgram(index) {
  if (program !== index) releaseNotes();
  program = index;
  for (const key of noteButtons.values()) key.disabled = !online || program !== 2;
  document.querySelector('#drum-controls').hidden = index !== 0;
  document.querySelector('#source-controls').hidden = index !== 1;
  const info = programs[index];
  document.querySelector('#program-name').textContent = info.name;
  document.querySelector('#program-number').textContent = `0${index} / 0${programs.length}`;
  document.querySelector('#program-description').textContent = info.description;
  document.querySelector('#led-description').textContent = info.led;
  document.querySelector('#button-guide').innerHTML = info.buttons.map((text, i) => `<div class="guide-row"><b>${i}</b><span>${text}</span></div>`).join('');
  info.pots.forEach((name, i) => {
    knobs[i].label.textContent = name;
    knobs[i].dial.setAttribute('aria-label', `Pot ${i}: ${name}`);
    knobs[i].slider.setAttribute('aria-label', `Pot ${i}: ${name} value`);
  });
  info.buttons.forEach((name, i) => {
    buttons[i].button.setAttribute('aria-label', `Button ${i}: ${name}`);
    buttons[i].label.textContent = i === 0 ? 'PROGRAM' : name.toUpperCase();
  });
}
async function poll() {
  try {
    const response = await fetch('/api/state', { cache: 'no-store', signal: AbortSignal.timeout(2000) });
    if (!response.ok) throw new Error('No state');
    const state = await response.json();
    if (!programs[state.program]) throw new Error('Unknown program');
    connection(true); document.querySelector('#error').hidden = true;
    if (program !== state.program) showProgram(state.program);
    updateDrums(state);
    state.pots.forEach((value, i) => {
      if (!knobs[i].dragging && performance.now() - knobs[i].editedAt > 250) paintKnob(i, value);
    });
    state.buttons.forEach((on, i) => buttons[i].button.classList.toggle('pressed', on || pressed.has(i)));
    state.leds.forEach((on, i) => leds[i].classList.toggle('on', on));
    const lit = state.leds.map((on, i) => on ? i : null).filter(i => i !== null);
    document.querySelector('#leds').setAttribute('aria-label', lit.length ? `LED ${lit.join(', ')} on` : 'All LEDs off');
    document.querySelector('#led-description').textContent = programs[state.program].led;
  } catch (_) {
    connection(false);
    for (const led of leds) led.classList.remove('on');
  }
  setTimeout(poll, online ? 25 : 1000);
}


// Keep the DSP clock on the host; schedule its stereo frames with Web Audio.
let audioContext = null, audioEnabled = false, audioCursor = null, audioTime = 0;
let audioGeneration = 0;
const audioToggle = document.querySelector('#audio-toggle');
const audioStatus = document.querySelector('#audio-status');
async function streamAudio(generation) {
  try {
    const response = await fetch(`/api/audio?cursor=${audioCursor ?? Number.MAX_SAFE_INTEGER}`, {
      cache: 'no-store', signal: AbortSignal.timeout(2000)
    });
    if (!response.ok) throw new Error('Audio stream unavailable');
    const samples = new Float32Array(await response.arrayBuffer());
    if (!audioEnabled || generation !== audioGeneration) return;
    audioCursor = Number(response.headers.get('X-Audio-Cursor'));
    const frames = samples.length / 2;
    if (frames && audioContext.state === 'running') {
      const buffer = audioContext.createBuffer(2, frames, 44100);
      for (let channel = 0; channel < 2; channel++) {
        const output = buffer.getChannelData(channel);
        for (let i = 0; i < frames; i++) output[i] = samples[2 * i + channel];
      }
      // Recover from stalls instead of queuing stale sound indefinitely.
      if (audioTime < audioContext.currentTime || audioTime > audioContext.currentTime + .25)
        audioTime = audioContext.currentTime + .07;
      const source = audioContext.createBufferSource();
      source.buffer = buffer; source.connect(audioContext.destination);
      source.start(audioTime); audioTime += frames / 44100;
    }
    audioStatus.textContent = audioContext.state === 'running' ? 'Stereo audio playing' : 'Audio paused by browser · click Disable, then Enable';
    setTimeout(() => streamAudio(generation), 20);
  } catch (error) {
    if (generation !== audioGeneration) return;
    audioEnabled = false;
    audioToggle.textContent = 'Enable audio';
    audioStatus.textContent = error.message;
    if (audioContext) { await audioContext.close(); audioContext = null; }
    releaseNotes();
  }
}
audioToggle.addEventListener('click', async () => {
  audioToggle.disabled = true;
  try {
    if (audioEnabled) {
      audioEnabled = false; audioGeneration++;
      await audioContext.close(); audioContext = null; releaseNotes();
      audioToggle.textContent = 'Enable audio'; audioStatus.textContent = 'Audio off';
    } else {
      audioStatus.textContent = 'Starting audio…';
      if (!audioContext) audioContext = new AudioContext({ latencyHint: 'interactive' });
      await audioContext.resume();
      audioEnabled = true; audioCursor = null; audioTime = 0;
      audioToggle.textContent = 'Disable audio';
      streamAudio(++audioGeneration);
    }
  } catch (error) { audioStatus.textContent = error.message; }
  finally { audioToggle.disabled = false; }
});

let octaveOffset = 0;
const noteOwners = new Map(), noteButtons = new Map();
const keyCodes = ['KeyA', 'KeyW', 'KeyS', 'KeyE', 'KeyD', 'KeyF', 'KeyT', 'KeyG', 'KeyY', 'KeyH', 'KeyU', 'KeyJ', 'KeyK'];
const noteNames = ['C4', 'C♯', 'D', 'D♯', 'E', 'F', 'F♯', 'G', 'G♯', 'A', 'A♯', 'B', 'C5'];
function noteDown(note, owner) {
  if (!online || program !== 2 || noteOwners.has(owner)) return;
  const alreadyHeld = [...noteOwners.values()].includes(note);
  noteOwners.set(owner, note);
  noteButtons.get(note).classList.add('held');
  noteButtons.get(note).setAttribute('aria-pressed', 'true');
  if (!alreadyHeld) sendControl({ type: 'note', index: note + octaveOffset * 12, velocity: 100 });
}
function noteUp(owner) {
  const note = noteOwners.get(owner);
  if (note === undefined) return;
  noteOwners.delete(owner);
  if (![...noteOwners.values()].includes(note)) {
    noteButtons.get(note).classList.remove('held');
    noteButtons.get(note).setAttribute('aria-pressed', 'false');
    sendControl({ type: 'note', index: note + octaveOffset * 12, velocity: 0 });
  }
}
function releaseNotes() { for (const owner of [...noteOwners.keys()]) noteUp(owner); }
for (let i = 0; i < 13; i++) {
  const note = 60 + i, button = document.createElement('button');
  button.type = 'button'; button.className = [1, 3, 6, 8, 10].includes(i) ? 'piano-key black' : 'piano-key';
  button.textContent = noteNames[i];
  button.setAttribute('aria-label', `${noteNames[i]}, MIDI note ${note}, ${keyCodes[i].slice(3)}`);
  button.setAttribute('aria-pressed', 'false');
  document.querySelector('#keyboard').append(button); noteButtons.set(note, button);
  button.addEventListener('pointerdown', event => {
    if (event.button !== 0) return;
    event.preventDefault(); button.focus(); button.setPointerCapture(event.pointerId);
    noteDown(note, `pointer-${event.pointerId}`);
  });
  for (const event of ['pointerup', 'pointercancel', 'lostpointercapture'])
    button.addEventListener(event, e => noteUp(`pointer-${e.pointerId}`));
  button.addEventListener('keydown', event => {
    if ([' ', 'Enter'].includes(event.key)) { event.preventDefault(); noteDown(note, `focus-${note}`); }
  });
  button.addEventListener('keyup', event => {
    if ([' ', 'Enter'].includes(event.key)) { event.preventDefault(); noteUp(`focus-${note}`); }
  });
  button.addEventListener('blur', () => noteUp(`focus-${note}`));
  button.addEventListener('click', event => {
    if (event.detail === 0 && !noteOwners.size) {
      noteDown(note, `assist-${note}`); setTimeout(() => noteUp(`assist-${note}`), 150);
    }
  });
}
function shiftOctave(amount) {
  releaseNotes();
  octaveOffset = Math.max(-5, Math.min(4, octaveOffset + amount));
  const low = 4 + octaveOffset;
  document.querySelector('#octave-range').textContent = `C${low}–C${low + 1}`;
  document.querySelector('#keyboard').setAttribute('aria-label', `MIDI keyboard C${low} to C${low + 1}`);
  document.querySelector('#octave-down').disabled = octaveOffset === -5;
  document.querySelector('#octave-up').disabled = octaveOffset === 4;
  for (let i = 0; i < 13; i++) {
    const button = noteButtons.get(60 + i);
    const name = i === 0 ? `C${low}` : i === 12 ? `C${low + 1}` : noteNames[i];
    button.textContent = name;
    button.setAttribute('aria-label', `${name}, MIDI note ${60 + i + octaveOffset * 12}, ${keyCodes[i].slice(3)}`);
  }
}
document.querySelector('#octave-down').addEventListener('click', () => shiftOctave(-1));
document.querySelector('#octave-up').addEventListener('click', () => shiftOctave(1));
window.addEventListener('keydown', event => {
  if (['KeyZ', 'KeyX'].includes(event.code) && !event.ctrlKey && !event.metaKey && !event.altKey && !event.target.matches('input, textarea, select, [contenteditable]')) {
    event.preventDefault();
    if (!event.repeat) shiftOctave(event.code === 'KeyZ' ? -1 : 1);
    return;
  }
  const index = keyCodes.indexOf(event.code);
  if (index < 0 || event.ctrlKey || event.metaKey || event.altKey || event.target.matches('input, textarea, select, [contenteditable]')) return;
  event.preventDefault(); if (!event.repeat) noteDown(60 + index, event.code);
});
window.addEventListener('keyup', event => noteUp(event.code));

// This source runs natively before Pepper; the browser only edits controls.
const bandNames = ['Kick', 'Tom', 'Snare', 'Conga', 'Claves', 'Perc', 'Hi-hat'];
const bandHz = [110, 220, 440, 880, 1760, 3520, 7040];
const eqControls = [];
const patternRows = [];
let drumState = null, sourceEditedAt = -Infinity, sourceDirty = false;
const sourceEnabled = document.querySelector('#source-enabled');
const sourceFrequency = document.querySelector('#source-frequency');
const sourceLevel = document.querySelector('#source-level');
const sourceWaveform = document.querySelector('#source-waveform');
const clockBpm = document.querySelector('#clock-bpm');
for (let i = 0; i < 7; i++) {
  const row = document.createElement('div'); row.className = 'eq-row';
  row.innerHTML = `<label for="eq-${i}">${bandNames[i]}<small>${bandHz[i]} Hz</small></label><input id="eq-${i}" type="range" min="-24" max="24" step="0.5" value="0"><output>0 dB</output><span class="band-activity" aria-label="${bandNames[i]} activity"><i></i></span>`;
  document.querySelector('#source-eq').append(row);
  const slider = row.querySelector('input'), output = row.querySelector('output');
  eqControls.push({slider, output, meter: row.querySelector('.band-activity i'), indicator: row.querySelector('.band-activity'), hits: 0});
  slider.addEventListener('input', () => {
    output.textContent = `${Number(slider.value) > 0 ? '+' : ''}${slider.value} dB`;
    editSource();
  });
}
function editSource() {
  sourceEditedAt = performance.now(); sourceDirty = true;
  document.querySelector('#source-level-value').textContent = `${Math.round(sourceLevel.value * 100)}%`;
}
function sendSource() {
  const frequency = Number(sourceFrequency.value);
  if (!Number.isFinite(frequency) || frequency < 35 || frequency > 8000) return;
  sendControl({type: 'source', enabled: sourceEnabled.checked, frequency,
    waveform: sourceWaveform.value, level: Number(sourceLevel.value), eq: eqControls.map(c => Number(c.slider.value))});
}
for (const control of [sourceEnabled, sourceFrequency, sourceLevel, sourceWaveform]) control.addEventListener('input', editSource);
setInterval(() => {
  if (sourceDirty && online) { sourceDirty = false; sendSource(); }
}, 35);
document.querySelector('#eq-flat').addEventListener('click', () => {
  eqControls.forEach(c => { c.slider.value = 0; c.output.textContent = '0 dB'; }); editSource();
});
document.querySelector('#drum-demo').addEventListener('click', () => {
  if (!online) return;
  if (!audioEnabled) audioToggle.click();
  if (program !== 0) { audioStatus.textContent = 'Select Sequencer with Button 0 for the drums'; return; }
  for (const [index, value] of [[0, (8-.4)/14], [4, .8], [5, 1]]) {
    paintKnob(index, value); knobs[index].editedAt = performance.now();
    sendControl({type: 'pot', index, value});
  }
  if (drumState?.external) sendControl({type: 'clock', bpm: Number(clockBpm.value), transport: 'start'});
});
for (const [id, opening] of [['kick-only', 0], ['hat-only', 1]]) {
  document.querySelector(`#${id}`).addEventListener('click', () => {
    document.querySelector('#drum-demo').click();
    editKnob(5, opening);
  });
}
let shownPattern = -1;
const voiceMeters = [];
function updateDrums(state) {
  drumState = state.drums;
  document.querySelector('#drum-pattern-name').textContent = `${state.drums.patternIndex+1} / ${state.drums.patterns.length} · ${state.drums.name}`;
  document.querySelector('#drum-status').textContent = `${state.drums.loaded}/${state.drums.sampleCount} samples · ${state.drums.kit} · filter ${Math.round(state.drums.filter*100)}% · ${state.drums.step < 0 ? 'waiting for step' : `step ${state.drums.step+1}/${state.drums.stepCount}`}`;
  document.querySelector('#clock-status').textContent = `${state.drums.external ? 'MIDI clock override' : 'Internal tempo'} · ${state.drums.bpm.toFixed(1)} BPM${state.clock.running ? ' · test clock running' : ''}`;
  if (performance.now() - sourceEditedAt > 500) {
    sourceEnabled.checked = state.source.enabled;
    sourceWaveform.value = state.source.waveform;
    if (document.activeElement !== sourceFrequency) sourceFrequency.value = Number(state.source.frequency.toFixed(1));
    sourceLevel.value = state.source.level;
    document.querySelector('#source-level-value').textContent = `${Math.round(state.source.level*100)}%`;
    eqControls.forEach((c, i) => {
      if (document.activeElement !== c.slider) c.slider.value = state.source.eq[i];
      c.output.textContent = `${state.source.eq[i] > 0 ? '+' : ''}${state.source.eq[i]} dB`;
    });
  }
  if (!voiceMeters.length) {
    for (let i=0; i<7; i++) {
      const row=document.createElement('div'); row.className='voice-level';
      const label=document.createElement('span'), meter=document.createElement('meter');
      meter.min=0; meter.max=1; row.append(label,meter);
      document.querySelector('#drum-voices').append(row); voiceMeters.push({label,meter,hits:0});
    }
  }
  voiceMeters.forEach((c,i) => {
    c.label.textContent=state.drums.voices[i]; c.meter.value=state.drums.intensities[i];
    c.meter.setAttribute('aria-label', `${state.drums.voices[i]} filter level`);
    if (c.hits !== state.drums.triggers[i]) {
      c.label.classList.add('hit'); setTimeout(()=>c.label.classList.remove('hit'),100);
    }
    c.hits=state.drums.triggers[i];
  });
  if (shownPattern !== state.drums.patternIndex) {
    shownPattern=state.drums.patternIndex; patternRows.length=0;
    document.querySelector('#drum-pattern').replaceChildren();
    state.drums.pattern.forEach(() => patternRows.push([]));
    for (let bar=0; bar<state.drums.stepCount/16; bar++) {
      const heading=document.createElement('div'); heading.className='pattern-bar-label';
      heading.textContent=`BAR ${bar+1}`;
      document.querySelector('#drum-pattern').append(heading);
      state.drums.pattern.forEach((notes, voice) => {
        const row=document.createElement('div'); row.className='pattern-row';
        const label=document.createElement('span'); label.textContent=state.drums.voices[voice]; row.append(label);
        notes.slice(bar*16,bar*16+16).forEach((accent,offset) => {
          const step=bar*16+offset;
          const cell=document.createElement('i'); cell.className='pattern-step';
          cell.classList.toggle('note',accent>0); cell.style.setProperty('--accent',accent);
          cell.title=`${state.drums.voices[voice]} · bar ${bar+1}, step ${offset+1}${accent ? ` · ${Math.round(accent*100)}% accent` : ' · rest'}`;
          row.append(cell); patternRows[voice].push(cell);
        });
        row.setAttribute('role','img');
        row.setAttribute('aria-label',`${state.drums.voices[voice]}, bar ${bar+1}, hits on steps ${notes.slice(bar*16,bar*16+16).flatMap((x,i)=>x?[i+1]:[]).join(', ')}`);
        document.querySelector('#drum-pattern').append(row);
      });
    }
  }
  patternRows.forEach((cells,voice)=>cells.forEach((cell,step)=>{
    cell.classList.toggle('current',step===state.drums.step);
    cell.classList.toggle('audible',step===state.drums.step && state.drums.velocities[voice]>0);
  }));
}
for (const transport of ['start', 'stop', 'continue']) {
  document.querySelector(`#clock-${transport}`).addEventListener('click', () => {
    const bpm = Number(clockBpm.value);
    if (bpm >= 20 && bpm <= 300) sendControl({type: 'clock', bpm, transport});
  });
}
clockBpm.addEventListener('change', () => {
  const bpm = Number(clockBpm.value);
  if (bpm >= 20 && bpm <= 300) sendControl({type: 'clock', bpm, transport: 'tempo'});
});

let midiAccess = null, midiPort = null, midiBytes = [];
const midiSelect = document.querySelector('#midi-input');
function chooseMidiInput() {
  if (midiPort) midiPort.onmidimessage = null;
  midiPort = midiAccess.inputs.get(midiSelect.value);
  if (midiPort) midiPort.onmidimessage = event => {
    if (midiBytes.length + event.data.length > 128) flushMidi();
    midiBytes.push(...event.data);
  };
  document.querySelector('#midi-status').textContent = midiPort ? `Receiving ${midiPort.name}. Enable Sync on Ableton’s corresponding output.` : 'No MIDI input selected';
}
function refreshMidiInputs() {
  const selected = midiSelect.value;
  midiSelect.replaceChildren(new Option('Choose input…', ''));
  for (const input of midiAccess.inputs.values()) midiSelect.add(new Option(input.name, input.id));
  midiSelect.value = selected;
  chooseMidiInput();
}
function flushMidi() {
  if (midiBytes.length && online) sendControl({type: 'midi', bytes: midiBytes.splice(0, 128)});
}
setInterval(flushMidi, 10);
midiSelect.addEventListener('change', chooseMidiInput);
document.querySelector('#midi-connect').addEventListener('click', async () => {
  try {
    if (!navigator.requestMIDIAccess) throw new Error('Web MIDI requires a supporting browser such as Chrome');
    midiAccess = await navigator.requestMIDIAccess({sysex: false});
    midiSelect.hidden = false; refreshMidiInputs();
    midiAccess.onstatechange = refreshMidiInputs;
  } catch (error) { document.querySelector('#midi-status').textContent = error.message; }
});

showProgram(0);
connection(false);
poll();
