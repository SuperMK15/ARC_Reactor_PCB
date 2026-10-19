let ws = null;
let isDragging = false;
let dragTimeout = null;

const connText = document.getElementById('connText');
const rngBright = document.getElementById('rngBright');
const rngSpeed = document.getElementById('rngSpeed');
const colCore = document.getElementById('colCore');
const colRing = document.getElementById('colRing');
const btnPower = document.getElementById('btnPower');
const btnUnibeam = document.getElementById('btnUnibeam');

function connectWS() {
    const host = window.location.hostname || '192.168.4.1';
    ws = new WebSocket(`ws://${host}/ws`);

    ws.onopen = () => connText.textContent = 'ONLINE';
    ws.onclose = () => { connText.textContent = 'OFFLINE'; setTimeout(connectWS, 1500); };
    ws.onmessage = (e) => {
        try { updateHUD(JSON.parse(e.data)); } catch (err) {}
    };
}

function sendCmd(cmd, value = null, extra = {}) {
    if (!ws || ws.readyState !== WebSocket.OPEN) return;
    ws.send(JSON.stringify({ cmd, value, ...extra }));
}

function updateHUD(data) {
    if (data.ssid) connText.textContent = data.ssid;

    if (Array.isArray(data.leds)) {
        data.leds.forEach((hex, idx) => {
            const el = document.getElementById(`led-${idx}`);
            if (el) {
                el.style.fill = hex;
                el.style.filter = `drop-shadow(0 0 ${idx === 0 ? 12 : 6}px ${hex})`;
            }
        });
    }

    if (!isDragging) {
        if (data.brightness !== undefined) rngBright.value = data.brightness;
        if (data.speed !== undefined) rngSpeed.value = data.speed;
        if (data.coreColor) colCore.value = data.coreColor;
        if (data.ringColor) colRing.value = data.ringColor;
    }

    if (data.power !== undefined) {
        btnPower.classList.toggle('active', data.power);
        btnPower.dataset.power = data.power ? '1' : '0';
    }

    if (data.mode !== undefined) {
        document.querySelectorAll('.mode-btn').forEach(btn => {
            btn.classList.toggle('active', parseInt(btn.dataset.mode) === data.mode);
        });
    }
}

document.querySelectorAll('.mode-btn').forEach(btn => {
    btn.addEventListener('click', () => sendCmd('setMode', parseInt(btn.dataset.mode)));
});

document.querySelectorAll('.preset-btn').forEach(btn => {
    btn.addEventListener('click', () => sendCmd('preset', parseInt(btn.dataset.preset)));
});

btnUnibeam.addEventListener('click', () => sendCmd('unibeam'));
btnPower.addEventListener('click', () => sendCmd('setPower', btnPower.dataset.power !== '1'));

function lockDrag() {
    isDragging = true;
    clearTimeout(dragTimeout);
    dragTimeout = setTimeout(() => { isDragging = false; }, 600);
}

rngBright.addEventListener('input', (e) => { lockDrag(); sendCmd('setBrightness', parseInt(e.target.value)); });
rngSpeed.addEventListener('input', (e) => { lockDrag(); sendCmd('setSpeed', parseInt(e.target.value)); });

function sendColors() {
    lockDrag();
    sendCmd('setColors', null, { core: colCore.value, ring: colRing.value });
}
colCore.addEventListener('input', sendColors);
colRing.addEventListener('input', sendColors);

window.addEventListener('DOMContentLoaded', connectWS);
