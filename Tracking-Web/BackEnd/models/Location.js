const mongoose = require('mongoose');

const locationSchema = new mongoose.Schema({
    device_id: { type: String, required: true },
    lat: { type: Number, required: true },
    lng: { type: Number, required: true },
    timestamp: { type: Date, default: Date.now, index: { expires: '30d' } } // TTL de 30 días
});

module.exports = mongoose.model('Location', locationSchema);
