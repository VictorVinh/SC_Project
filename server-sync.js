const express = require('express');
const fs = require('fs');
const path = require('path');
const app = express();
app.use(express.json());
const DATA = path.join(__dirname,'players.json');

function readData(){
  try { return JSON.parse(fs.readFileSync(DATA)); }
  catch(e){ return []; }
}
function writeData(arr){ fs.writeFileSync(DATA, JSON.stringify(arr, null, 2)); }

// POST /player -> cập nhật hoặc thêm player
app.post('/player', (req,res) => {
  const p = req.body;
  if(!p || !p.name) return res.status(400).json({err:'invalid'});
  let arr = readData();
  const idx = arr.findIndex(x => x.name === p.name && x.class === p.class);
  const now = new Date().toISOString();
  if(idx === -1) {
    arr.push({ name: p.name, class: p.class || '', status: p.status || '', score: p.score || 0, updatedAt: now});
  } else {
    arr[idx].status = p.status || arr[idx].status;
    arr[idx].score = (typeof p.score === 'number') ? p.score : arr[idx].score;
    arr[idx].updatedAt = now;
  }
  writeData(arr);
  res.json({ok:true});
});

// GET /leaderboard.json -> trả JSON array, sort giảm dần theo score
app.get('/leaderboard.json', (req,res) => {
  let arr = readData();
  arr.sort((a,b) => (b.score||0) - (a.score||0));
  res.json(arr);
});

// optional HTML view
app.get('/leaderboard', (req,res) => {
  let arr = readData(); arr.sort((a,b) => (b.score||0) - (a.score||0));
  let html = '<html><body><h2>Leaderboard</h2><table border=1><tr><th>#</th><th>Name</th><th>Class</th><th>Score</th><th>Status</th></tr>';
  arr.forEach((p,i)=> html += `<tr><td>${i+1}</td><td>${p.name}</td><td>${p.class}</td><td>${p.score}</td><td>${p.status}</td></tr>`);
  html += '</table></body></html>';
  res.send(html);
});

app.listen(3000, ()=> console.log('Server running on :3000'));
