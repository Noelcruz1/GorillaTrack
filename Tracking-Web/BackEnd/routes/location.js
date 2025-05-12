const express = require('express');
const router = express.Router();
const Location = require('../routes/Location');

const AUTH_TOKEN = process.env.AUTH_TOKEN || "12345";

// POST /api/location
router.post('/', async (req, res) => {
    const authHeader = req.headers.authorization;
    if (!authHeader || authHeader !== `Bearer ${AUTH_TOKEN}`) {
        return res.status(401).json({ error: "Token no válido" });
    }

    const { device_id, lat, lng, timestamp } = req.body;
    if (!device_id || !lat || !lng) {
        return res.status(400).json({ error: "Faltan datos requeridos" });
    }

    try {
        const location = new Location({ device_id, lat, lng, timestamp });
        await location.save();
        res.status(201).json({ message: "Ubicación guardada con éxito" });
    } catch (err) {
        res.status(500).json({ error: "Error al guardar ubicación" });
    }
});

// GET /api/location?id=Gori-1234
router.get('/', async (req, res) => {
    const { id } = req.query;
    if (!id) return res.status(400).json({ error: "Falta el ID del dispositivo" });

    try {
        const locations = await Location.find({ device_id: id }).sort({ timestamp: 1 });
        res.json(locations);
    } catch (err) {
        res.status(500).json({ error: "Error al obtener ubicaciones" });
    }
});

module.exports = router;
