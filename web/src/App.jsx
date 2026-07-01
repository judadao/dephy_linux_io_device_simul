import React, { useMemo, useState } from "react";
import {
  Activity,
  Braces,
  Cable,
  ListRestart,
  Play,
  RotateCcw,
  SkipForward,
  TerminalSquare,
  Trash2
} from "lucide-react";
import devicePanel from "./assets/device-panel.svg";

const initialChannels = [
  { name: "door", type: "di", value: 0, tMs: 0 },
  { name: "relay", type: "do", value: 0, tMs: 0 },
  { name: "pressure", type: "ai", value: 0, tMs: 0 },
  { name: "valve", type: "ao", value: 0, tMs: 0 }
];

const defaultScript = `sleep 10
set door 1
sleep 5
set pressure 1234
write relay 1
sleep 10
set door 0`;

const site = "demo";
const node = "linux-sim-001";

function isInput(type) {
  return type === "di" || type === "ai";
}

function isOutput(type) {
  return type === "do" || type === "ao";
}

function topicFor(channelName) {
  return `site/${site}/node/${node}/io/${channelName}/event`;
}

function nextEventName(channel, nextValue, write) {
  if (write) {
    return "write";
  }
  if (channel.type === "di" && channel.value === 0 && nextValue !== 0) {
    return "rising";
  }
  if (channel.type === "di" && channel.value !== 0 && nextValue === 0) {
    return "falling";
  }
  return "changed";
}

function cleanScriptLines(script) {
  return script
    .split("\n")
    .map((line) => line.trim())
    .filter((line) => line.length > 0 && !line.startsWith("#"));
}

export default function App() {
  const [channels, setChannels] = useState(initialChannels);
  const [script, setScript] = useState(defaultScript);
  const [nowMs, setNowMs] = useState(0);
  const [lineIndex, setLineIndex] = useState(0);
  const [events, setEvents] = useState([]);
  const [status, setStatus] = useState("idle");

  const lines = useMemo(() => cleanScriptLines(script), [script]);
  const latest = events[0];
  const latestTopic = latest ? topicFor(latest.channel) : "site/demo/node/linux-sim-001/io/-/event";
  const latestPayload = latest ? JSON.stringify(latest, null, 2) : "{}";

  function pushEvent(channel, eventName, timeMs) {
    const event = {
      event: eventName,
      channel: channel.name,
      type: channel.type,
      value: channel.value,
      fault: 0,
      t_ms: timeMs
    };
    setEvents((current) => [event, ...current].slice(0, 120));
  }

  function applyLine(line, timeMs) {
    const parts = line.split(/\s+/);
    const op = parts[0];

    if (op === "sleep" && parts.length === 2) {
      const delta = Number(parts[1]);
      if (!Number.isFinite(delta) || delta < 0) {
        return { ok: false, timeMs };
      }
      return { ok: true, timeMs: timeMs + delta };
    }

    if (op === "poll") {
      return { ok: true, timeMs };
    }

    if ((op === "set" || op === "write") && parts.length === 3) {
      const channelName = parts[1];
      const value = Number(parts[2]);
      const write = op === "write";
      let emitted = null;
      let valid = true;

      if (!Number.isFinite(value)) {
        return { ok: false, timeMs };
      }

      setChannels((current) => current.map((channel) => {
        if (channel.name !== channelName) {
          return channel;
        }
        if ((write && !isOutput(channel.type)) || (!write && !isInput(channel.type))) {
          valid = false;
          return channel;
        }
        const next = { ...channel, value, tMs: timeMs };
        emitted = {
          channel: next,
          eventName: nextEventName(channel, value, write)
        };
        return next;
      }));

      if (!valid || !emitted) {
        return { ok: false, timeMs };
      }
      pushEvent(emitted.channel, emitted.eventName, timeMs);
      return { ok: true, timeMs };
    }

    return { ok: false, timeMs };
  }

  function stepScript() {
    if (lineIndex >= lines.length) {
      setStatus("done");
      return false;
    }

    const result = applyLine(lines[lineIndex], nowMs);
    setNowMs(result.timeMs);
    setLineIndex((index) => index + 1);
    setStatus(result.ok ? `line ${lineIndex + 1}/${lines.length}` : "error");
    return result.ok;
  }

  function runScript() {
    let timeMs = nowMs;
    let index = lineIndex;
    let ok = true;

    setStatus("running");
    while (index < lines.length && ok) {
      const result = applyLine(lines[index], timeMs);
      timeMs = result.timeMs;
      ok = result.ok;
      index += 1;
    }
    setNowMs(timeMs);
    setLineIndex(index);
    setStatus(ok ? "done" : "error");
  }

  function resetSimulator() {
    setChannels(initialChannels);
    setNowMs(0);
    setLineIndex(0);
    setEvents([]);
    setStatus("idle");
  }

  return (
    <main className="shell">
      <section className="device-band" aria-label="Device overview">
        <div className="device-copy">
          <p className="eyebrow">Local Linux simulator</p>
          <h1>Linux IO Device Simulator</h1>
          <div className="device-meta">
            <span>site/{site}</span>
            <span>node/{node}</span>
            <span>t={nowMs}ms</span>
          </div>
        </div>
        <img className="device-art" src={devicePanel} alt="Rack mounted simulator panel" />
      </section>

      <section className="workspace" aria-label="Simulator workspace">
        <div className="panel io-panel">
          <div className="panel-head">
            <h2><Activity size={18} /> IO State</h2>
            <button className="icon-button" type="button" onClick={resetSimulator} title="Reset simulator">
              <RotateCcw size={17} />
              <span>Reset</span>
            </button>
          </div>
          <div className="io-grid">
            {channels.map((channel) => (
              <div className="io-row" key={channel.name}>
                <span className={`type-badge type-${channel.type}`}>{channel.type.toUpperCase()}</span>
                <span>
                  <span className="channel-name">{channel.name}</span>
                  <span className="channel-time">t={channel.tMs}ms</span>
                </span>
                <span className="value-box">{channel.value}</span>
              </div>
            ))}
          </div>
        </div>

        <div className="panel script-panel">
          <div className="panel-head">
            <h2><TerminalSquare size={18} /> Script</h2>
            <div className="button-row">
              <button type="button" onClick={stepScript} title="Run one line">
                <SkipForward size={17} />
                <span>Step</span>
              </button>
              <button type="button" onClick={runScript} title="Run script">
                <Play size={17} />
                <span>Run</span>
              </button>
            </div>
          </div>
          <textarea
            id="scriptEditor"
            value={script}
            spellCheck="false"
            onChange={(event) => {
              setScript(event.target.value);
              setLineIndex(0);
              setStatus("edited");
            }}
          />
        </div>

        <div className="panel event-panel">
          <div className="panel-head">
            <h2><ListRestart size={18} /> Event Stream</h2>
            <button type="button" onClick={() => setEvents([])} title="Clear events">
              <Trash2 size={17} />
              <span>Clear</span>
            </button>
          </div>
          <ol className="event-list">
            {events.map((event, index) => (
              <li className="event-item" key={`${event.channel}-${event.t_ms}-${index}`}>
                <span className={`event-${event.event}`}>{event.event}</span>
                <strong>{event.channel}</strong>
                <span>{event.value}</span>
              </li>
            ))}
          </ol>
        </div>
      </section>

      <section className="protocol-band" aria-label="Protocol output">
        <div className="panel payload-panel">
          <div className="panel-head">
            <h2><Cable size={18} /> Latest Topic</h2>
            <span className="status-pill">{status}</span>
          </div>
          <code>{latestTopic}</code>
        </div>
        <div className="panel payload-panel">
          <div className="panel-head">
            <h2><Braces size={18} /> Latest Payload</h2>
          </div>
          <pre>{latestPayload}</pre>
        </div>
      </section>
    </main>
  );
}
