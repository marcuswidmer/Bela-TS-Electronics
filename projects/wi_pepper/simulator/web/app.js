'use strict';

const programs = [
  { name: 'Sequencer', description: 'A changing sequence of notes, with speed, pattern and variation at your fingertips.',
    pots: ['Speed', 'Pattern', 'Variation', 'Unused', 'Unused', 'Unused', 'Unused', 'Unused'],
    buttons: ['Next program', 'Resync', 'Unused', 'Unused'],
    led: 'LED 9 · step & priority countdown' },
  { name: 'KarplusResonator', description: 'Twelve resonating strings. Shape their tuning, decay and texture. Audio connections will come later.',
    pots: ['Volume', 'Excitation', 'Decay', 'Brightness', 'Transpose', 'Dry / wet', 'Detune', 'Width'],
    buttons: ['Next program', 'Note set', 'Sustain', 'Clear'],
    led: 'LEDs 0–9 · input meter (no audio connected)' },
  { name: 'JUNO', description: 'Six voices, warm oscillator layers and stereo chorus. Connect a MIDI keyboard to the real Pepper to play. Desktop audio and MIDI remain disconnected.',
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
function releaseAll() { for (const i of pressed.keys()) release(i); }
window.addEventListener('blur', releaseAll);
document.addEventListener('visibilitychange', () => { if (document.hidden) releaseAll(); });

function showProgram(index) {
  program = index;
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
    state.pots.forEach((value, i) => {
      if (!knobs[i].dragging && performance.now() - knobs[i].editedAt > 250) paintKnob(i, value);
    });
    state.buttons.forEach((on, i) => buttons[i].button.classList.toggle('pressed', on || pressed.has(i)));
    state.leds.forEach((on, i) => leds[i].classList.toggle('on', on));
    const lit = state.leds.map((on, i) => on ? i : null).filter(i => i !== null);
    document.querySelector('#leds').setAttribute('aria-label', lit.length ? `LED ${lit.join(', ')} on` : 'All LEDs off');
    document.querySelector('#led-description').textContent = state.indicating ? `LED ${state.program} · four program flashes` : programs[state.program].led;
  } catch (_) {
    connection(false);
    for (const led of leds) led.classList.remove('on');
  }
  setTimeout(poll, online ? 25 : 1000);
}
showProgram(0);
connection(false);
poll();
