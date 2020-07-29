#pragma once

enum RadioRecvCode
{
    None,
    // Pitch i roll volana. Ovu opciju bi trebalo da koristi samo vozilo, a ne volan.
    WheelPos = 101,
    // Obrtanje vozila u mestu.
    Spin,
    // Pauza u komunikaciji izmedju volana i vozila.
    Pause,
    // Kraj upravljanja vozila volanom.
    End,
};