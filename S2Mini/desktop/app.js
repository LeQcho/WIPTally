const $ = id => document.getElementById(id);
let role = 'master', token = '', running = false, starting = false, simulation = false, connected = false, localError = false;
let closed = false;
let portLists = {atem: [], sony: []};
const saved = (() => {try {return JSON.parse(localStorage.getItem('tally-atelier') || '{}');} catch {return {};}})();
const savedFields = ['ssid', 'atem', 'inputs', 'camera', 'leds', 'rgb-order', 'sony-name', 'sony-mac', 'sony-pin', 'sony-pixels', 'sony-order'];
for (const name of savedFields) if (saved[name] !== undefined) $(name).value = saved[name];
for (let n = 1; n <= 16; n++) {
  const button = document.createElement('button'); button.type = 'button'; button.textContent = n;
  button.addEventListener('click', () => { $('camera').value = n; update(); });
  $('camera-chips').append(button);
}
function update() {
  const master = role === 'master', sony = role === 'sony-c3';
  setPorts(sony ? portLists.sony : portLists.atem);
  $('master-fields').hidden = !master; $('master-fields').disabled = !master || running || starting;
  $('receiver-fields').hidden = master || sony; $('receiver-fields').disabled = master || sony || running || starting;
  $('sony-fields').hidden = !sony; $('sony-fields').disabled = !sony || running || starting;
  for (const [id, active] of [['nav-master', master], ['nav-receiver', !master && !sony], ['nav-sony', sony]]) {
    $(id).classList.toggle('active', active); $(id).setAttribute('aria-pressed', active);
  }
  $('mode-label').textContent = sony ? 'SONY / BLUETOOTH LE' : 'ATEM / ESP-NOW';
  $('board-tag').textContent = sony ? 'C3 MINI' : 'S2 MINI';
  $('device-board').innerHTML = sony ? 'C3<br>MINI' : 'S2<br>MINI';
  $('board-footer').textContent = sony ? 'Sony REC · LOLIN C3 Mini · expérimental' : 'Wemos LOLIN S2 Mini · ESP32-S2';
  $('page-title').innerHTML = master ? 'Le point de départ<br>de tes tally.' : 'Une caméra.<br>Un récepteur prêt.';
  $('page-description').textContent = master ? 'Prépare le master qui relie ton ATEM à toutes les caméras.' : 'Choisis sa caméra, son éclairage, et mémorise le tout en un flash.';
  $('form-title').textContent = master ? 'Configurer le master' : 'Configurer le récepteur';
  $('summary-title').textContent = master ? 'Master ATEM' : `Caméra ${$('camera').value || '—'}`;
  $('summary-detail').textContent = master ? `${$('inputs').value || '—'} entrées caméra · Wi-Fi` : ($('leds').value === 'receiver-rgb' ? '2 LED RGB WS2812 · ESP-NOW' : 'LED classique · ESP-NOW');
  $('summary-firmware').textContent = master ? 'Master' : $('leds').value === 'receiver-rgb' ? 'Récepteur RGB' : 'Récepteur classique';
  $('config-note').textContent = master ? 'Le master rejoint le Wi-Fi. Les récepteurs communiquent avec lui directement en ESP-NOW.' : 'Aucun mot de passe Wi-Fi sur le récepteur. Il retrouve automatiquement le master et mémorise son numéro de caméra.';
  if (sony) {
    $('page-title').innerHTML = 'Le REC du Sony.<br>Visible sur ton tally.';
    $('page-description').textContent = 'Un voyant d’enregistrement autonome, directement relié à ta caméra en Bluetooth.';
    $('form-title').textContent = 'Configurer Tally Sony';
    $('summary-title').textContent = 'Tally Sony REC';
    $('summary-detail').textContent = `${$('sony-name').value || 'Sony'} · ${$('sony-pixels').value || '—'} LED · GPIO${$('sony-pin').value}`;
    $('summary-firmware').textContent = 'Sony C3 · expérimental';
    $('config-note').textContent = 'Déplace le fil DATA des LED sur le GPIO choisi de la C3 Mini. GPIO18 est réservé à son USB. Le profil écoute les états REC sans déclencher d’enregistrement.';
  }
  const rgb = !master && !sony && $('leds').value === 'receiver-rgb';
  for (const id of ['rgb-order','rgb-order-label']) $(id).hidden = !rgb;
  $('receiver-fields').querySelector('.color-key').hidden = !rgb;
  $('password').disabled = $('open-wifi').checked;
  $('led-hint').textContent = $('leds').value === 'receiver-rgb' ? 'Les deux RGB affichent le même état. Caméra au repos : éteintes. ATEM indisponible : bleu clignotant.' : 'La LED intégrée reste fixe en Programme et clignote en Preview. Sorties externes rouge GPIO16 et verte GPIO17.';
  $('flash').replaceChildren(document.createTextNode(simulation ? 'Simuler le flash' : sony ? 'Flasher Tally Sony' : master ? 'Flasher le master' : 'Flasher le récepteur'));
  const arrow = document.createElement('span'); arrow.textContent = '↗'; $('flash').append(arrow);
  for (const id of ['flash', 'check', 'refresh', 'nav-master', 'nav-receiver', 'nav-sony', 'port', 'quit']) $(id).disabled = running || starting || !connected;
  $('flash').disabled ||= !$('port').value;
  if (sony && $('sony-board').value !== 'lolin_c3_mini') {
    $('flash').disabled = true; $('check').disabled = true;
    $('config-note').textContent = 'La S2 Mini n’a pas de Bluetooth. Pour ce profil Sony, utilise une LOLIN C3 Mini. Les profils ATEM restent disponibles dans les autres onglets.';
  }
  for (const button of $('camera-chips').children) button.classList.toggle('active', Number(button.textContent) === Number($('camera').value));
  try { localStorage.setItem('tally-atelier', JSON.stringify({...Object.fromEntries(savedFields.map(id => [id, $(id).value]))})); } catch {}
}
$('nav-master').addEventListener('click', () => {role = 'master'; update();});
$('nav-receiver').addEventListener('click', () => {role = 'receiver'; update();});
$('nav-sony').addEventListener('click', () => {role = 'sony-c3'; update();});
$('settings').addEventListener('submit', event => event.preventDefault());
$('settings').addEventListener('input', update);
$('toggle-password').addEventListener('click', () => {const show = $('password').type === 'password'; $('password').type = show ? 'text' : 'password'; $('toggle-password').textContent = show ? 'Masquer' : 'Afficher'; $('toggle-password').setAttribute('aria-label', show ? 'Masquer le mot de passe' : 'Afficher le mot de passe');});
$('toggle-log').addEventListener('click', () => {const show = $('log').hidden; $('log').hidden = !show; $('toggle-log').textContent = show ? 'Masquer le journal ↑' : 'Afficher le journal ↓'; $('toggle-log').setAttribute('aria-expanded', show);});
$('port').addEventListener('change', update);
function setPorts(list) {
  const before = $('port').value;
  const signature = JSON.stringify([role, list]);
  if ($('port').dataset.signature === signature) return;
  $('port').dataset.signature = signature;
  $('port').replaceChildren();
  const board = role === 'sony-c3' ? 'C3 Mini' : 'S2 Mini';
  if (list.length !== 1) { const option = document.createElement('option'); option.value = ''; option.textContent = list.length ? 'Choisir la carte à flasher…' : `Aucune ${board} détectée`; $('port').append(option); }
  for (const port of list) {const option = document.createElement('option'); option.value = port.device; option.textContent = `${port.description} · ${port.device}`; $('port').append(option);}
  if (list.some(port => port.device === before)) $('port').value = before;
  $('port-hint').textContent = list.length ? `${list.length} carte${list.length > 1 ? 's' : ''} détectée${list.length > 1 ? 's' : ''}. Vérifie la carte cible avant de flasher.` : `Branche une ${board} avec un câble USB de données, puis actualise.`;
}
async function poll() {
  if (closed) return;
  try {
    const response = await fetch('/api/state', {cache: 'no-store'}); if (!response.ok) throw new Error('Serveur local indisponible');
    const data = await response.json();
    $('build-info').textContent = `Application ${data.version || 'ancienne'} · firmware master ${(data.firmware?.master || '').slice(0,8)}`;
    token = data.token; connected = true; simulation = data.simulate;
    $('simulation').hidden = !simulation; portLists = {atem: data.ports, sony: data.ports_sony || []};
    const job = data.job; running = job.status === 'running';
    if (job.status !== 'idle' && !starting && !localError) {
      $('status-title').textContent = {running: 'Opération en cours…', success: 'Opération terminée', error: 'Opération à reprendre'}[job.status];
      $('status-message').textContent = job.message;
      $('status-dot').className = `status-dot ${job.status}`;
      $('progress').hidden = !running;
      $('log').textContent = job.lines.join('\n'); $('log').scrollTop = $('log').scrollHeight;
    }
    update();
  } catch {
    connected = false; update(); $('status-title').textContent = 'Application locale déconnectée';
    $('status-message').textContent = 'Relance Tally Atelier depuis son lanceur. Ne débranche pas la carte si un flash est encore en cours.';
    $('status-dot').className = 'status-dot error'; $('progress').hidden = true;
  }
}
async function start(checkOnly) {
  if (running || starting || !connected) return;
  if (!$('settings').reportValidity()) return;
  const sony = role === 'sony-c3';
  const body = sony ? {role, port:$('port').value, board:$('sony-board').value, sony_name:$('sony-name').value, sony_mac:$('sony-mac').value.trim(), sony_pin:$('sony-pin').value, sony_pixels:$('sony-pixels').value, rgb_order:$('sony-order').value} :
    {role: role === 'master' ? 'master' : $('leds').value, port: $('port').value,
    ssid: $('ssid').value, password: $('open-wifi').checked ? '' : $('password').value, atem: $('atem').value, inputs: $('inputs').value, camera: $('camera').value, open_wifi:$('open-wifi').checked, rgb_order:$('rgb-order').value};
  starting = true; localError = false; update();
  $('status-title').textContent = 'Préparation…'; $('status-message').textContent = 'Vérification du firmware et des réglages.';
  try {
    const response = await fetch(checkOnly ? '/api/check' : '/api/flash', {method: 'POST', headers: {'Content-Type':'application/json','X-Tally-Token':token}, body:JSON.stringify(body)});
    const result = await response.json(); if (!response.ok) throw new Error(result.error || 'Opération impossible');
    // Keep the password in this page's memory for repeat flashes, never localStorage.
    running = true;
  } catch (error) {
    localError = true;
    $('status-title').textContent = 'Vérifie les réglages'; $('status-message').textContent = error.message;
    $('status-dot').className = 'status-dot error';
  } finally { starting = false; update(); }
  // A failed submission must remain visible; only poll immediately when accepted.
  if (running) await poll();
}
$('flash').addEventListener('click', () => start(false));
$('check').addEventListener('click', () => start(true));
$('refresh').addEventListener('click', poll);
$('quit').addEventListener('click', async () => {
  if (running || starting) return;
  try {
    const response = await fetch('/api/quit', {method:'POST',headers:{'X-Tally-Token':token}});
    if (!response.ok) throw new Error();
    closed = true; connected = false; update();
    $('status-title').textContent = 'Tally Atelier est fermée';
    $('status-message').textContent = 'Tu peux fermer cet onglet. Pour revenir, double-clique sur le lanceur Tally Atelier.';
    $('status-dot').className = 'status-dot';
  } catch { $('status-message').textContent = 'Impossible de quitter pour le moment. Attendre la fin du flash.'; }
});
update(); poll(); setInterval(poll, 1000);
