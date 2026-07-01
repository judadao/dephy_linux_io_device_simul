import React, { useEffect, useMemo, useRef, useState } from "react";
import {
  Braces,
  Cable,
  Download,
  FileJson,
  Infinity,
  ListRestart,
  Pause,
  Plus,
  Play,
  RotateCcw,
  SkipForward,
  TerminalSquare,
  Trash2,
  Upload,
  X,
  Zap
} from "lucide-react";
import devicePanel from "./assets/device-panel.svg";

const ioTypes = {
  di: { id: "di", label: "DI", mode: "digital", title: "Digital Input" },
  do: { id: "do", label: "DO", mode: "digital", title: "Digital Output" },
  ai: { id: "ai", label: "AI", mode: "analog", title: "Analog Input" },
  ao: { id: "ao", label: "AO", mode: "analog", title: "Analog Output" },
  relay: { id: "relay", label: "RELAY", mode: "digital", title: "Relay Output" }
};

const initialModules = [
  { slotNo: 1, type: "di" },
  { slotNo: 2, type: "do" },
  { slotNo: 3, type: "ai" },
  { slotNo: 4, type: "ao" },
  { slotNo: 5, type: "relay" },
  { slotNo: 6, type: "di" },
  { slotNo: 7, type: "do" },
  { slotNo: 8, type: "ai" },
  { slotNo: 9, type: "ao" },
  { slotNo: 10, type: "relay" },
  { slotNo: 11, type: "di" },
  { slotNo: 12, type: "do" },
  { slotNo: 13, type: "ai" },
  { slotNo: 14, type: "ao" },
  { slotNo: 15, type: "relay" },
  { slotNo: 16, type: "di" },
  { slotNo: 17, type: "do" },
  { slotNo: 18, type: "ai" },
  { slotNo: 19, type: "ao" },
  { slotNo: 20, type: "relay" }
];

const defaultScript = `slot6 DI 1 1
slot1 DI 2 1
slot3 AI 1 128
slot2 DO 3 1
slot5 RELAY 1 1
sleep 100
slot1/DI/1/0
slot5/RELAY/1/0`;

const ioScriptTest = `slot1 DI 1 1
slot3 AI 2 42
slot5 RELAY 3 1`;

const importExportRunTestPath = "/local_trigger_scenarios/import_export_run.json";
const triggerImportTestPath = "/local_trigger_scenarios/twenty_slot_interleaved.trigger";

const site = "demo";
const node = "linux-sim-001";
const maxSlots = 20;

function buildInitialSlots() {
  return initialModules.flatMap((module) => buildModuleSlots(module.slotNo, module.type));
}

function buildModuleSlots(slotNo, typeId) {
  const type = ioTypes[typeId];
  if (!type) {
    return [];
  }
  return (
    Array.from({ length: 16 }, (_, index) => ({
      key: `slot${slotNo}-${type.id}-${index + 1}`,
      type: type.id,
      label: type.label,
      mode: type.mode,
      point: index + 1,
      slotNo,
      value: 0,
      tMs: 0
    }))
  );
}

function modulesFromSlots(slots) {
  const seen = new Set();
  return slots
    .filter((slot) => {
      if (seen.has(slot.slotNo)) {
        return false;
      }
      seen.add(slot.slotNo);
      return true;
    })
    .map((slot) => ({ slotNo: slot.slotNo, type: slot.type }))
    .sort((a, b) => a.slotNo - b.slotNo);
}

function cleanScriptLines(script) {
  return script
    .split("\n")
    .map((line) => line.trim())
    .filter((line) => line.length > 0 && !line.startsWith("#"));
}

function isLit(slot) {
  return slot.value > 0;
}

function topicFor(slot) {
  return `site/${site}/node/${node}/slot/${slot.slotNo}/io/${slot.label.toLowerCase()}/${slot.point}/event`;
}

function eventNameFor(slot, value) {
  if (slot.mode === "digital" && slot.value === 0 && value > 0) {
    return "rising";
  }
  if (slot.mode === "digital" && slot.value > 0 && value === 0) {
    return "falling";
  }
  return "changed";
}

export default function App() {
  const [slots, setSlots] = useState(buildInitialSlots);
  const slotsRef = useRef(slots);
  const [script, setScript] = useState(defaultScript);
  const [nowMs, setNowMs] = useState(0);
  const [lineIndex, setLineIndex] = useState(0);
  const [events, setEvents] = useState([]);
  const [status, setStatus] = useState("idle");
  const [newSlotType, setNewSlotType] = useState("di");
  const [loopCount, setLoopCount] = useState(1);
  const [infiniteLoop, setInfiniteLoop] = useState(false);
  const [running, setRunning] = useState(false);
  const importInputRef = useRef(null);
  const runTimerRef = useRef(null);
  const stopRunRef = useRef(false);
  const runnerRef = useRef({
    lines: [],
    index: 0,
    loop: 1,
    totalLoops: 1,
    timeMs: 0
  });

  const lines = useMemo(() => cleanScriptLines(script), [script]);
  const modules = useMemo(() => modulesFromSlots(slots), [slots]);
  const latest = events[0];
  const latestTopic = latest ? latest.topic : "site/demo/node/linux-sim-001/io/-/-/event";
  const latestPayload = latest ? JSON.stringify(latest.payload, null, 2) : "{}";

  useEffect(() => {
    const testMode = new URLSearchParams(window.location.search).get("test");

    if (testMode === "io-script") {
      replaceSlots(buildInitialSlots());
      setEvents([]);

      let timeMs = 0;
      let ok = true;
      const testLines = cleanScriptLines(ioScriptTest);

      for (const line of testLines) {
        const result = applyLine(line, timeMs);
        timeMs = result.timeMs;
        ok = ok && result.ok;
      }

      setScript(ioScriptTest);
      setNowMs(timeMs);
      setLineIndex(testLines.length);
      setStatus(ok ? "script test done" : "script test error");
      return;
    }

    if (testMode === "import-export-run") {
      fetch(importExportRunTestPath)
        .then((response) => response.json())
        .then((config) => {
          if (!applySceneConfig(config)) {
            setStatus("import export run test error");
            return;
          }

          const exported = sceneConfigFromState(config.script || "");
          const exportOk = exported.modules.length > 0 &&
            exported.channels.some((channel) => Number(channel.value) > 0) &&
            typeof exported.script === "string" &&
            exported.script.includes("slot");
          let timeMs = 0;
          let runOk = exportOk;
          const testLines = cleanScriptLines(exported.script);

          for (const line of testLines) {
            const result = applyLine(line, timeMs);
            timeMs = result.timeMs;
            runOk = runOk && result.ok;
          }

          setNowMs(timeMs);
          setLineIndex(testLines.length);
          setStatus(runOk ? "import export run test done" : "import export run test error");
        })
        .catch(() => setStatus("import export run test error"));
      return;
    }

    if (testMode === "trigger-import") {
      fetch(triggerImportTestPath)
        .then((response) => response.text())
        .then((text) => {
          setScript(text);
          setLineIndex(0);
          setStatus(text.includes("slot20 RELAY 5 1") ? "trigger import test done" : "trigger import test error");
        })
        .catch(() => setStatus("trigger import test error"));
    }
  }, []);

  function replaceSlots(nextSlots) {
    slotsRef.current = nextSlots;
    setSlots(nextSlots);
  }

  function sceneConfigFromState(scriptText = script) {
    return {
      version: 1,
      name: "linux-io-device-simul-scene",
      modules: modulesFromSlots(slotsRef.current),
      channels: slotsRef.current.map((slot) => ({
        slot: slot.slotNo,
        type: slot.label,
        channel: slot.point,
        value: slot.value
      })),
      script: scriptText
    };
  }

  function exportSceneConfig() {
    const config = sceneConfigFromState();
    const blob = new Blob([JSON.stringify(config, null, 2)], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");

    link.href = url;
    link.download = "linux-io-sim-scene.json";
    document.body.appendChild(link);
    link.click();
    link.remove();
    URL.revokeObjectURL(url);
    setStatus("exported");
  }

  function applySceneConfig(config) {
    const importedModules = Array.isArray(config.modules) ? config.modules : initialModules;
    const normalizedModules = importedModules
      .map((module) => ({
        slotNo: Number(module.slotNo),
        type: String(module.type).toLowerCase()
      }))
      .filter((module) => Number.isInteger(module.slotNo) &&
        module.slotNo >= 1 &&
        module.slotNo <= maxSlots &&
        ioTypes[module.type]);
    const importedSlots = normalizedModules.flatMap((module) => buildModuleSlots(module.slotNo, module.type));
    const channelValues = Array.isArray(config.channels) ? config.channels : [];
    const nextSlots = importedSlots.map((slot) => {
      const match = channelValues.find((channel) =>
        Number(channel.slot) === slot.slotNo &&
        String(channel.type).toLowerCase() === slot.type &&
        Number(channel.channel) === slot.point
      );
      if (!match) {
        return slot;
      }
      const value = Number(match.value);
      return {
        ...slot,
        value: Number.isFinite(value) ? value : 0
      };
    });

    if (nextSlots.length === 0) {
      return false;
    }

    replaceSlots(nextSlots);
    setScript(typeof config.script === "string" ? config.script : defaultScript);
    setNowMs(0);
    setLineIndex(0);
    setEvents([]);
    return true;
  }

  function importSceneConfig(file) {
    const reader = new FileReader();

    reader.onload = () => {
      const text = String(reader.result || "");
      const trimmed = text.trim();

      if (!trimmed) {
        setStatus("import error");
        return;
      }

      if (!trimmed.startsWith("{")) {
        setScript(text);
        setLineIndex(0);
        setStatus("script imported");
        return;
      }

      try {
        const config = JSON.parse(trimmed);
        setStatus(applySceneConfig(config) ? "imported" : "import error");
      } catch {
        setStatus("import error");
      }
    };

    reader.readAsText(file);
  }

  function pushEvent(slot, value, eventName, timeMs) {
    const event = {
      event: eventName,
      topic: topicFor(slot),
      payload: {
        event: eventName,
        slot_type: slot.label,
        slot: slot.slotNo,
        point: slot.point,
        value,
        lamp: value > 0 ? "on" : "off",
        fault: 0,
        t_ms: timeMs
      }
    };
    setEvents((current) => [event, ...current].slice(0, 100));
  }

  function setSlotValue(slotNoText, typeLabel, pointText, valueText, timeMs) {
    const slotNo = Number(slotNoText);
    const type = typeLabel.toLowerCase();
    const point = Number(pointText);
    const value = Number(valueText);
    const currentSlots = slotsRef.current;
    const slot = currentSlots.find((item) => item.slotNo === slotNo && item.type === type && item.point === point);

    if (!Number.isInteger(slotNo) || slotNo < 1 || slotNo > maxSlots ||
        !Number.isInteger(point) || point < 1 || point > 16 ||
        !Number.isFinite(value) || !slot) {
      return false;
    }

    const normalizedValue = slot.mode === "digital" ? (value > 0 ? 1 : 0) : value;
    const eventName = eventNameFor(slot, normalizedValue);

    replaceSlots(currentSlots.map((slot) => {
      if (slot.slotNo !== slotNo || slot.type !== type || slot.point !== point) {
        return slot;
      }
      return {
        ...slot,
        value: normalizedValue,
        tMs: timeMs
      };
    }));

    pushEvent(slot, normalizedValue, eventName, timeMs);
    return true;
  }

  function addModule(slotNoText, typeLabel) {
    const slotNo = Number(slotNoText);
    const type = typeLabel.toLowerCase();
    const currentSlots = slotsRef.current;

    if (!Number.isInteger(slotNo) || slotNo < 1 || slotNo > maxSlots || !ioTypes[type]) {
      return false;
    }
    if (currentSlots.some((slot) => slot.slotNo === slotNo)) {
      return false;
    }

    replaceSlots([...currentSlots, ...buildModuleSlots(slotNo, type)]);
    setStatus(`added slot${slotNo}`);
    return true;
  }

  function removeModule(slotNoText) {
    const slotNo = Number(slotNoText);
    const currentSlots = slotsRef.current;

    if (!Number.isInteger(slotNo) || !currentSlots.some((slot) => slot.slotNo === slotNo)) {
      return false;
    }

    replaceSlots(currentSlots.filter((slot) => slot.slotNo !== slotNo));
    setStatus(`removed slot${slotNo}`);
    return true;
  }

  function addNextModule() {
    const used = new Set(modules.map((module) => module.slotNo));
    let slotNo = 1;
    while (used.has(slotNo)) {
      slotNo += 1;
    }
    if (slotNo > maxSlots) {
      setStatus("max 20 slots");
      return;
    }
    addModule(String(slotNo), newSlotType);
  }

  function applyLine(line, timeMs) {
    const normalized = line.replace(/\//g, " ");
    const parts = normalized.split(/\s+/);
    const op = parts[0].toLowerCase();

    if (op === "sleep" && parts.length === 2) {
      const delta = Number(parts[1]);
      if (!Number.isFinite(delta) || delta < 0) {
        return { ok: false, timeMs };
      }
      return { ok: true, timeMs: timeMs + delta };
    }

    if (op === "trigger" && parts.length === 4) {
      const slotType = Object.values(ioTypes).find((type) => type.id === parts[1].toLowerCase());
      if (!slotType) {
        return { ok: false, timeMs };
      }
      const module = modulesFromSlots(slotsRef.current).find((item) => item.type === slotType.id);
      if (!module) {
        return { ok: false, timeMs };
      }
      return {
        ok: setSlotValue(String(module.slotNo), parts[1], parts[2], parts[3], timeMs),
        timeMs
      };
    }

    if (op === "add" && parts.length === 3 && parts[1].toLowerCase().startsWith("slot")) {
      return {
        ok: addModule(parts[1].slice(4), parts[2]),
        timeMs
      };
    }

    if (op === "remove" && parts.length === 2 && parts[1].toLowerCase().startsWith("slot")) {
      return {
        ok: removeModule(parts[1].slice(4)),
        timeMs
      };
    }

    if (op === "slot" && parts.length === 5) {
      return {
        ok: setSlotValue(parts[1], parts[2], parts[3], parts[4], timeMs),
        timeMs
      };
    }

    if (op.startsWith("slot") && parts.length === 4) {
      return {
        ok: setSlotValue(op.slice(4), parts[1], parts[2], parts[3], timeMs),
        timeMs
      };
    }

    return { ok: false, timeMs };
  }

  function clearRunTimer() {
    if (runTimerRef.current) {
      clearTimeout(runTimerRef.current);
      runTimerRef.current = null;
    }
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

  function startLoopRun() {
    const runLines = cleanScriptLines(script);
    const totalLoops = infiniteLoop ? Infinity : Math.max(1, Number(loopCount) || 1);

    if (runLines.length === 0) {
      setStatus("empty script");
      return;
    }

    clearRunTimer();
    stopRunRef.current = false;
    runnerRef.current = {
      lines: runLines,
      index: 0,
      loop: 1,
      totalLoops,
      timeMs: nowMs
    };
    setRunning(true);
    setStatus(infiniteLoop ? "looping" : `loop 1/${totalLoops}`);
    setLineIndex(0);

    const tick = () => {
      const runner = runnerRef.current;

      if (stopRunRef.current) {
        setRunning(false);
        setStatus("stopped");
        return;
      }

      if (runner.index >= runner.lines.length) {
        if (runner.loop >= runner.totalLoops) {
          setRunning(false);
          setStatus("done");
          setLineIndex(runner.lines.length);
          return;
        }
        runner.loop += 1;
        runner.index = 0;
        setStatus(runner.totalLoops === Infinity ? `loop ${runner.loop}` : `loop ${runner.loop}/${runner.totalLoops}`);
      }

      const line = runner.lines[runner.index];
      const before = runner.timeMs;
      const result = applyLine(line, runner.timeMs);
      const delay = Math.max(0, Math.min(result.timeMs - before, 10000));

      runner.timeMs = result.timeMs;
      setNowMs(runner.timeMs);
      setLineIndex(runner.index + 1);

      if (!result.ok) {
        setRunning(false);
        setStatus("error");
        return;
      }

      runner.index += 1;
      runTimerRef.current = setTimeout(tick, delay);
    };

    tick();
  }

  function stopLoopRun() {
    stopRunRef.current = true;
    clearRunTimer();
    setRunning(false);
    setStatus("stopped");
  }

  function clickSlot(slot) {
    const nextValue = slot.mode === "digital" ? (slot.value > 0 ? 0 : 1) : slot.value > 0 ? 0 : 100;
    setSlotValue(String(slot.slotNo), slot.label, String(slot.point), String(nextValue), nowMs);
    setStatus("manual");
  }

  function resetSimulator() {
    stopLoopRun();
    replaceSlots(buildInitialSlots());
    setNowMs(0);
    setLineIndex(0);
    setEvents([]);
    setStatus("idle");
  }

  return (
    <main className="shell">
      <section className="device-band compact" aria-label="Device overview">
        <div className="device-copy">
          <p className="eyebrow">16-point local IO simulator</p>
          <h1>Linux IO Slot Simulator</h1>
          <div className="device-meta">
            <span>slotX / io type / channel / value</span>
            <span>add/remove module slots, max 20</span>
            <span>t={nowMs}ms</span>
          </div>
        </div>
        <img className="device-art" src={devicePanel} alt="Rack mounted simulator panel" />
      </section>

      <section className="slot-board" aria-label="IO slot board">
        {modules.map((module) => {
          const type = ioTypes[module.type];
          const typeSlots = slots.filter((slot) => slot.slotNo === module.slotNo);
          const litCount = typeSlots.filter(isLit).length;

          return (
            <div className={`slot-card type-${type.id}`} key={module.slotNo}>
              <div className="slot-card-head">
                <div>
                  <h2>S{module.slotNo} {type.label}</h2>
                  <span>{type.label} - {type.title}</span>
                </div>
                <div className="slot-actions">
                  <strong>{litCount}/16 ON</strong>
                  <button type="button" onClick={() => removeModule(String(module.slotNo))} title={`Remove slot ${module.slotNo}`}>
                    <X size={15} />
                  </button>
                </div>
              </div>
              <div className="point-grid">
                {typeSlots.map((slot) => (
                  <button
                    className={`point ${isLit(slot) ? "point-on" : ""}`}
                    key={slot.key}
                    type="button"
                    onClick={() => clickSlot(slot)}
                    title={`${slot.label} ${slot.point}: ${slot.value}`}
                  >
                    <span className="lamp" />
                    <span className="point-id">{slot.point}</span>
                    <span className="point-value">{slot.value}</span>
                  </button>
                ))}
              </div>
            </div>
          );
        })}
      </section>

      <section className="module-bar" aria-label="Module controls">
        <span><FileJson size={17} /> Scene config</span>
        <select value={newSlotType} onChange={(event) => setNewSlotType(event.target.value)}>
          {Object.values(ioTypes).map((type) => (
            <option value={type.id} key={type.id}>{type.label}</option>
          ))}
        </select>
        <button type="button" onClick={addNextModule}>
          <Plus size={17} />
          <span>Add next slot</span>
        </button>
        <button type="button" onClick={exportSceneConfig}>
          <Download size={17} />
          <span>Export</span>
        </button>
        <button type="button" onClick={() => importInputRef.current?.click()}>
          <Upload size={17} />
          <span>Import</span>
        </button>
        <input
          ref={importInputRef}
          className="hidden-file"
          type="file"
          accept="application/json,.json,.trigger,.txt,text/plain"
          onChange={(event) => {
            const file = event.target.files?.[0];
            if (file) {
              importSceneConfig(file);
            }
            event.target.value = "";
          }}
        />
      </section>

      <section className="workspace slot-workspace" aria-label="Simulator controls">
        <div className="panel script-panel">
          <div className="panel-head">
            <h2><TerminalSquare size={18} /> Trigger Script</h2>
            <div className="button-row">
              <button type="button" onClick={stepScript} title="Run one line">
                <SkipForward size={17} />
                <span>Step</span>
              </button>
              <button type="button" onClick={running ? stopLoopRun : startLoopRun} title={running ? "Stop loop" : "Start loop"}>
                {running ? <Pause size={17} /> : <Play size={17} />}
                <span>{running ? "Stop" : "Start Loop"}</span>
              </button>
              <button type="button" onClick={resetSimulator} title="Reset simulator">
                <RotateCcw size={17} />
                <span>Reset</span>
              </button>
            </div>
          </div>
          <div className="loop-controls">
            <label>
              <span>Loop</span>
              <input
                type="number"
                min="1"
                max="999"
                value={loopCount}
                disabled={infiniteLoop}
                onChange={(event) => setLoopCount(event.target.value)}
              />
            </label>
            <label className="check-control">
              <input
                type="checkbox"
                checked={infiniteLoop}
                onChange={(event) => setInfiniteLoop(event.target.checked)}
              />
              <Infinity size={17} />
              <span>Infinite</span>
            </label>
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
              <li className="event-item slot-event" key={`${event.topic}-${event.payload.t_ms}-${index}`}>
                <span className={`event-${event.payload.event}`}>{event.payload.event}</span>
                <strong>S{event.payload.slot}/{event.payload.slot_type}/{event.payload.point}</strong>
                <span>{event.payload.value}</span>
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

      <section className="usage-strip" aria-label="Script command reference">
        <Zap size={18} />
        <code>add slot6 DI</code>
        <code>slot1 DI 1 1</code>
        <code>slot3 AI 1 128</code>
        <code>slot5/RELAY/1/0</code>
        <code>remove slot6</code>
      </section>
    </main>
  );
}
