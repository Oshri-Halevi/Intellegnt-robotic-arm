import { useEffect, useState } from "react";
import Slider from "./mySlider.jsx";

export default function App() {
  // === Config ===
  const ARDUINO_IP = "192.168.4.1";

  // Presets used for Base/Drop actions (Catch uses Arduino sequence only)
  const SERVO_POSITIONS = {
    initial: [
      { name: "Gripper",     num: 6, min: 0,  max: 140, best: 140 },
      { name: "Wrist Pitch", num: 5, min: 0,  max: 180, best: 102 },
      { name: "Wrist Roll",  num: 4, min: 0,  max: 180, best:   0 },
      { name: "Elbow",       num: 3, min: 80, max: 180, best:  90 },
      { name: "Shoulder",    num: 2, min: 0,  max: 180, best:  82 },
      { name: "Waist",       num: 1, min: 0,  max: 180, best: 130 },
    ],
    base: [
      { name: "Gripper",     num: 6, min: 0,  max: 140, best: 140 },
      { name: "Wrist Pitch", num: 5, min: 0,  max: 180, best: 102 },
      { name: "Wrist Roll",  num: 4, min: 0,  max: 180, best:   0 },
      { name: "Elbow",       num: 3, min: 80, max: 180, best: 120 },
      { name: "Shoulder",    num: 2, min: 0,  max: 180, best: 100 },
      { name: "Waist",       num: 1, min: 0,  max: 180, best: 124 },
    ],
    // Catch is driven by Arduino sequence; we don't preset sliders here.
    dropRed: [
      // Matches RED_STEPS: shoulder 154, waist 75, elbow 168, gripper 102
      { name: "Gripper",     num: 6, min: 0,  max: 140, best: 102 },
      { name: "Wrist Pitch", num: 5, min: 0,  max: 180, best: 102 },
      { name: "Wrist Roll",  num: 4, min: 0,  max: 180, best:   0 },
      { name: "Elbow",       num: 3, min: 80, max: 180, best: 168 },
      { name: "Shoulder",    num: 2, min: 0,  max: 180, best: 154 },
      { name: "Waist",       num: 1, min: 0,  max: 180, best:  75 },
    ],
    dropBlack: [
      // Matches BLACK_STEPS: shoulder 145, waist 50, elbow 135, gripper 102
      { name: "Gripper",     num: 6, min: 0,  max: 140, best: 102 },
      { name: "Wrist Pitch", num: 5, min: 0,  max: 180, best: 102 },
      { name: "Wrist Roll",  num: 4, min: 0,  max: 180, best:   0 },
      { name: "Elbow",       num: 3, min: 80, max: 180, best: 135 },
      { name: "Shoulder",    num: 2, min: 0,  max: 180, best: 145 },
      { name: "Waist",       num: 1, min: 0,  max: 180, best:  50 }, 
    ],
  };

  // === State ===
  const [servos, setServos] = useState(SERVO_POSITIONS.initial);
  const [savedPositions, setSavedPositions] = useState(() => {
    const saved = localStorage.getItem("savedPositions");
    return saved ? JSON.parse(saved) : [];
  });
  const [hoveredPositionId, setHoveredPositionId] = useState(null);
  const [statusMsg, setStatusMsg] = useState("");

  // Persist saved positions
  useEffect(() => {
    localStorage.setItem("savedPositions", JSON.stringify(savedPositions));
  }, [savedPositions]);

  // === Handlers ===
  const updateServo = (num, newValue) => {
    setServos((prev) =>
      prev.map((s) => (s.num === num ? { ...s, best: newValue } : s))
    );
  };

  const savePosition = () => {
    const name = prompt("Enter a name for this position:");
    if (name) setSavedPositions((prev) => [...prev, { name, servos, id: Date.now() }]);
  };

  const deletePosition = (idToDelete) => {
    setSavedPositions((prev) => prev.filter((p) => p.id !== idToDelete));
  };

  const moveToPosition = async (savedServos) => {
    setServos(savedServos);
    for (const s of savedServos) {
      try {
        await fetch(`http://${ARDUINO_IP}/servo?num=${s.num}&angle=${s.best}`);
      } catch (err) {
        console.error(`Failed to move ${s.name}:`, err?.message || err);
      }
    }
  };

  // For Base/Drop: trigger Arduino action and update UI to the final pose
  const handleAction = async (command, finalPositions) => {
    if (finalPositions) setServos(finalPositions);
    try {
      await fetch(`http://${ARDUINO_IP}/action?cmd=${command}`);
    } catch (err) {
      console.error(`Failed action ${command}:`, err?.message || err);
    }
  };

  // Catch follows Arduino sequence exactly (CATCH_STEPS). No UI preset.
  const runCatchSequence = async () => {
    setStatusMsg("Running catch sequence…");
    try {
      await fetch(`http://${ARDUINO_IP}/action?cmd=catchCube`);
    } catch (err) {
      console.error("Failed action catchCube:", err?.message || err);
    } finally {
      setTimeout(() => setStatusMsg(""), 2000);
    }
  };

  // === UI ===
  return (
    <div className="flex w-full p-8 font-sans gap-8 min-h-screen bg-gray-900 text-gray-200 justify-center">
      {/* Middle: Sliders + actions */}
      <div className="w-1/2 max-w-2xl">
        <h1 className="text-3xl font-bold mb-6 text-center">Arduino Servo Control</h1>

        <div className="space-y-4">
          {servos.map((servo) => (
            <Slider
              key={servo.num}
              name={servo.name}
              min={servo.min}
              max={servo.max}
              value={servo.best}
              servoNum={servo.num}
              arduinoIP={ARDUINO_IP}
              onChange={updateServo}
            />
          ))}
        </div>

        <div className="mt-8 grid grid-cols-2 gap-4">
          <button
            className="px-4 py-2 rounded-lg shadow bg-blue-600 text-white font-semibold hover:bg-blue-700 transition"
            onClick={() => handleAction("base", SERVO_POSITIONS.base)}
          >
            Go to Base
          </button>

          {/* Catch uses Arduino sequence (CATCH_STEPS) */}
          <button
            className="px-4 py-2 rounded-lg shadow bg-blue-600 text-white font-semibold hover:bg-blue-700 transition"
            onClick={runCatchSequence}
          >
            Catch Cube
          </button>

          <button
            className="px-4 py-2 rounded-lg shadow bg-blue-600 text-white font-semibold hover:bg-blue-700 transition"
            onClick={() => handleAction("dropIntoRed", SERVO_POSITIONS.dropRed)}
          >
            Drop into Red
          </button>
          <button
            className="px-4 py-2 rounded-lg shadow bg-blue-600 text-white font-semibold hover:bg-blue-700 transition"
            onClick={() => handleAction("dropIntoBlack", SERVO_POSITIONS.dropBlack)}
          >
            Drop into Black
          </button>
          <button
            className="col-span-2 px-4 py-2 rounded-lg shadow bg-green-600 text-white font-semibold hover:bg-green-700 transition"
            onClick={savePosition}
          >
            Save Current Position
          </button>
        </div>
        {statusMsg && (
          <div className="mt-3 text-center text-sm text-blue-300">{statusMsg}</div>
        )}
      </div>

      {/* Right: Saved positions */}
      <div className="w-1/4 max-w-sm bg-gray-800 p-4 rounded-lg shadow-inner flex flex-col">
        <h2 className="text-xl font-bold mb-4 border-b border-gray-600 pb-2 flex-shrink-0">
          Saved Positions
        </h2>
        <div className="space-y-3 overflow-y-auto flex-grow">
          {savedPositions.length ? (
            savedPositions.map((pos) => (
              <div
                key={pos.id}
                className="relative flex justify-between items-center bg-gray-700/50 p-2 rounded"
                onMouseEnter={() => setHoveredPositionId(pos.id)}
                onMouseLeave={() => setHoveredPositionId(null)}
              >
                <span
                  className="cursor-pointer hover:text-blue-400 truncate pr-2 text-base"
                  onClick={() => setServos(pos.servos)}
                >
                  {pos.name}
                </span>

                {hoveredPositionId === pos.id && (
                  <div className="absolute left-1/2 -translate-x-1/2 top-full mt-2 w-max bg-gray-900 border border-gray-600 p-2 rounded-lg shadow-lg z-10">
                    <ul className="text-xs text-gray-300 space-y-1">
                      {pos.servos.map((s) => (
                        <li key={s.num} className="flex justify-between gap-4">
                          <span className="font-semibold">{s.name}:</span>
                          <span>{s.best}°</span>
                        </li>
                      ))}
                    </ul>
                  </div>
                )}

                <div className="flex gap-2 flex-shrink-0">
                  <button
                    onClick={() => moveToPosition(pos.servos)}
                    className="text-xs bg-green-600 hover:bg-green-700 text-white font-bold py-1 px-2 rounded transition-colors"
                  >
                    Go To
                  </button>
                  <button
                    onClick={() => deletePosition(pos.id)}
                    className="text-xs bg-red-600 hover:bg-red-700 text-white font-bold py-1 px-2 rounded transition-colors"
                  >
                    Delete
                  </button>
                </div>
              </div>
            ))
          ) : (
            <p className="text-gray-400">No saved positions.</p>
          )}
        </div>
      </div>
    </div>
  );
}