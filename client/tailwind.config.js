/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
   theme: {
    extend: {
      colors: {
        brand: {
          light_orange: "#FE7743", 
          dark_blue: "#273F4F", 
          light_grey: "#EFEEEA",
          black:"#000000"  
        },
        accent: {
          green: "#A3DC9A",
          red: "#FF6363",
        }
      }
    },
  },
  plugins: [],
}
//https://colorhunt.co/palette/fe7743efeeea273f4f000000
