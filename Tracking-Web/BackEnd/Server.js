require('dotenv').config();
const express = require('express');
const mongoose = require('mongoose');
const locationRoutes = require('../routes/location');

const app = express();
app.use(express.json());

// Conexión MongoDB Atlas
mongoose.connect(process.env.MONGO_URI, {
    useNewUrlParser: true,
    useUnifiedTopology: true
}).then(() => console.log("✅ Conectado a MongoDB"))
  .catch(err => console.error("❌ Error en MongoDB:", err));

// Rutas
app.use('/api/location', locationRoutes);

// Página principal
app.get('/', (req, res) => {
    res.send('GorilaTrack API funcionando 🦍📍');
});

// Puerto
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => console.log(`🚀 Servidor en puerto ${PORT}`));
