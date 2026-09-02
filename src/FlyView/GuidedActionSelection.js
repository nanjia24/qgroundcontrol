.pragma library

function snapshot(selectedVehicles) {
    const vehicles = []
    if (!selectedVehicles) {
        return vehicles
    }

    for (let i = 0; i < selectedVehicles.count; i++) {
        const vehicle = selectedVehicles.get(i)
        vehicles.push({ key: vehicleKey(vehicle), vehicle: vehicle })
    }
    return vehicles
}

function vehicleKey(vehicle) {
    if (!vehicle) {
        return undefined
    }
    return vehicle.vehicleId !== undefined ? vehicle.vehicleId : vehicle.id
}

function matches(selectedVehicles, expectedVehicles) {
    if (!selectedVehicles || !expectedVehicles || selectedVehicles.count !== expectedVehicles.length) {
        return false
    }

    const matchedExpectedVehicles = []
    for (let i = 0; i < expectedVehicles.length; i++) {
        matchedExpectedVehicles.push(false)
    }

    for (let i = 0; i < selectedVehicles.count; i++) {
        const currentVehicle = selectedVehicles.get(i)
        let found = false
        for (let j = 0; j < expectedVehicles.length; j++) {
            if (!matchedExpectedVehicles[j] && vehicleKey(currentVehicle) === expectedVehicles[j].key) {
                matchedExpectedVehicles[j] = true
                found = true
                break
            }
        }
        if (!found) {
            return false
        }
    }
    return true
}

function describe(selectedVehicles, expectedVehicles) {
    const currentKeys = []
    if (selectedVehicles) {
        for (let i = 0; i < selectedVehicles.count; i++) {
            currentKeys.push(vehicleKey(selectedVehicles.get(i)))
        }
    }
    const expectedKeys = []
    if (expectedVehicles) {
        for (let i = 0; i < expectedVehicles.length; i++) {
            expectedKeys.push(expectedVehicles[i].key)
        }
    }
    return "current=[" + currentKeys.join(",") + "], expected=[" + expectedKeys.join(",") + "]"
}
