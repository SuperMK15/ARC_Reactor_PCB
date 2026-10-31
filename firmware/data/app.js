let ws = null;
let isDragging = false;
let dragTimeout = null;
let reconnectTimeout = null;

const connText = document.getElementById('connText');
const rngBright = document.getElementById('rngBright');
const rngSpeed = document.getElementById('rngSpeed');
const brightnessValue = document.getElementById('brightnessValue');
const speedValue = document.getElementById('speedValue');
const currentDraw = document.getElementById('currentDraw');
const activeMode = document.getElementById('activeMode');
const colCore = document.getElementById('colCore');
const colRing = document.getElementById('colRing');
const btnPower = document.getElementById('btnPower');
const btnUnibeam = document.getElementById('btnUnibeam');
const ledNodes = Array.from({ length: 9 }, (_, idx) => document.getElementById(`led-${idx}`));
const modeNames = ['Classic', 'Turbine', 'Helix', 'Heartbeat', 'Overload', 'Synthwave', 'Static', 'Standby'];

function setConnectionState(state, label) {
    connText.dataset.state = state;
    connText.lastChild.textContent = ` ${label}`;
}

function connectWS() {
    if (ws && (ws.readyState === WebSocket.CONNECTING || ws.readyState === WebSocket.OPEN)) return;

    const host = window.location.hostname || '192.168.4.1';
    setConnectionState('connecting', 'CONNECTING');
    ws = new WebSocket(`ws://${host}/ws`);

    ws.onopen = () => setConnectionState('online', 'ONLINE');
    ws.onclose = () => {
        setConnectionState('offline', 'RECONNECTING');
        clearTimeout(reconnectTimeout);
        reconnectTimeout = setTimeout(connectWS, 1500);
    };
    ws.onerror = (error) => {
        console.error('WebSocket connection error:', error);
        ws.close();
    };
    ws.onmessage = (event) => {
        try {
            updateHUD(JSON.parse(event.data));
        } catch (error) {
            console.error('Could not process reactor telemetry:', error);
        }
    };
}

function sendCmd(cmd, value = null, extra = {}) {
    if (!ws || ws.readyState !== WebSocket.OPEN) return;
    ws.send(JSON.stringify({ cmd, value, ...extra }));
}

function updateLEDs(colors) {
    let estimatedMilliamps = 0;
    colors.forEach((hex, idx) => {
        const led = ledNodes[idx];
        if (!led || !/^#[\da-f]{6}$/i.test(hex)) return;

        led.style.fill = hex;
        led.style.filter = `drop-shadow(0 0 ${idx === 0 ? 13 : 8}px ${hex})`;
        estimatedMilliamps += [1, 3, 5].reduce((sum, offset) => {
            return sum + parseInt(hex.slice(offset, offset + 2), 16);
        }, 0) * (20 / 255);
    });
    currentDraw.innerHTML = `${Math.round(estimatedMilliamps)} <small>mA</small>`;
}

function updateHUD(data) {
    if (Array.isArray(data.leds)) updateLEDs(data.leds);

    if (!isDragging) {
        if (data.brightness !== undefined) {
            rngBright.value = data.brightness;
            brightnessValue.innerHTML = `${data.brightness} <small>/ 255</small>`;
        }
        if (data.speed !== undefined) {
            rngSpeed.value = data.speed;
            speedValue.innerHTML = `${data.speed} <small>/ 100</small>`;
        }
        if (data.coreColor) colCore.value = data.coreColor;
        if (data.ringColor) colRing.value = data.ringColor;
    }

    if (data.power !== undefined) {
        btnPower.classList.toggle('active', data.power);
        btnPower.dataset.power = data.power ? '1' : '0';
        btnPower.setAttribute('aria-label', data.power ? 'Turn reactor off' : 'Turn reactor on');
        btnPower.title = data.power ? 'Turn reactor off' : 'Turn reactor on';
    }

    if (data.mode !== undefined && modeNames[data.mode]) {
        activeMode.textContent = modeNames[data.mode];
        document.querySelectorAll('.mode-btn').forEach((button) => {
            button.classList.toggle('active', Number(button.dataset.mode) === data.mode);
        });
    }
}

document.querySelectorAll('.mode-btn').forEach((button) => {
    button.addEventListener('click', () => sendCmd('setMode', Number(button.dataset.mode)));
});

document.querySelectorAll('.preset-btn').forEach((button) => {
    button.addEventListener('click', () => sendCmd('preset', Number(button.dataset.preset)));
});

btnUnibeam.addEventListener('click', () => sendCmd('unibeam'));
btnPower.addEventListener('click', () => sendCmd('setPower', btnPower.dataset.power !== '1'));

function lockDrag() {
    isDragging = true;
    clearTimeout(dragTimeout);
    dragTimeout = setTimeout(() => { isDragging = false; }, 600);
}

rngBright.addEventListener('input', (event) => {
    lockDrag();
    brightnessValue.innerHTML = `${event.target.value} <small>/ 255</small>`;
    sendCmd('setBrightness', Number(event.target.value));
});

rngSpeed.addEventListener('input', (event) => {
    lockDrag();
    speedValue.innerHTML = `${event.target.value} <small>/ 100</small>`;
    sendCmd('setSpeed', Number(event.target.value));
});

function sendColors() {
    lockDrag();
    sendCmd('setColors', null, { core: colCore.value, ring: colRing.value });
}

colCore.addEventListener('input', sendColors);
colRing.addEventListener('input', sendColors);

window.addEventListener('DOMContentLoaded', connectWS);
