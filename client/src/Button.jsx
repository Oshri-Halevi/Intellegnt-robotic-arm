export default function ControlButton({ text, onClick }) {
  return (
    <button
      onClick={onClick}
      className="px-4 py-2 rounded-lg shadow bg-blue-600 text-white font-semibold hover:bg-blue-700 transition"
    >
      {text}
    </button>
  );
}
