import { useEffect, useState } from "react";

export default function Slider({
  name,
  min,
  max,
  value,
  servoNum,
  num,
  arduinoIP,
  onChange,
}) {
  const id = servoNum ?? num;
  const [internalAngle, setInternalAngle] = useState(value);

  // keep in sync if parent changes 
  useEffect(() => {
    if (value !== internalAngle) setInternalAngle(value);
   
  }, [value]);

  // debounced send to Arduino
  useEffect(() => {
    if (id == null) return;
    const t = setTimeout(() => {
      fetch(`http://${arduinoIP}/servo?num=${id}&angle=${internalAngle}`).catch(
        (err) => console.error(`Failed to move ${name}:`, err?.message || err)
      );
    }, 120);
    return () => clearTimeout(t);
  }, [internalAngle, id, arduinoIP, name]);

  const handleDrag = (e) => {
    const newAngle = parseInt(e.target.value, 10);
    setInternalAngle(newAngle);
    if (typeof onChange === "function" && id != null) {
      onChange(id, newAngle);
    }
  };

  return (
    <div className="flex flex-col bg-white/5 rounded-xl p-4 shadow-md">
      <label className="font-semibold mb-2 flex justify-between">
        <span>{name}</span>
        <span>{internalAngle}°</span>
      </label>
      <input
        type="range"
        min={min}
        max={max}
        value={internalAngle}
        onChange={handleDrag}
        className="w-full accent-blue-500 cursor-pointer"
      />
      {id == null && (
        <p className="text-red-400 text-xs mt-2">
          Missing servo ID (pass <code>servoNum</code> or <code>num</code>).
        </p>
      )}
    </div>
  );
}
