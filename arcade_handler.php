<?php
// Permitir peticiones desde Live Server (CORS)
header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Methods: GET, POST, OPTIONS");
header("Access-Control-Allow-Headers: Content-Type");

// Configuración técnica
error_reporting(E_ALL);
ini_set('display_errors', 1);

$host = "localhost"; 
$user = "root";
$pass = ""; 
$db   = "arcade_nfc";
$port = 3307; // Restaurado a 3307 (importante para tu servidor de BD)

$archivo = __DIR__ . "/sesion_activa.txt"; // Ruta absoluta para evitar fallos de lectura/escritura

// 1. GESTIÓN DE ARCHIVOS (Sin requerir BD)
if (isset($_POST['limpiar_sesion'])) {
    file_put_contents($archivo, "");
    echo "OK: Archivo vaciado";
    exit;
}

if (isset($_POST['registrar_inicio'])) {
    $uid = $_POST['uid'];
    file_put_contents($archivo, $uid);
    echo "Sesión local abierta [$uid]";
    // Seguir para intentar DB...
}

// 2. CONEXIÓN A BASE DE DATOS
try {
    $conn = new mysqli($host, $user, $pass, "", $port);
    if ($conn->connect_error) {
        throw new Exception("Fallo de conexión: " . $conn->connect_error);
    }

    $conn->query("CREATE DATABASE IF NOT EXISTS $db");
    $conn->select_db($db);

    $conn->query("CREATE TABLE IF NOT EXISTS jugadores (
        uid_hex VARCHAR(20) PRIMARY KEY,
        puntos_totales INT DEFAULT 0,
        ultima_sesion TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
    )");

    $conn->query("CREATE TABLE IF NOT EXISTS partidas (
        id INT AUTO_INCREMENT PRIMARY KEY,
        uid_hex VARCHAR(20),
        puntos_ganados INT,
        fecha TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )");

    if (isset($_POST['registrar_inicio'])) {
        $uid = $_POST['uid'];
        $stmt = $conn->prepare("INSERT INTO jugadores (uid_hex) VALUES (?) ON DUPLICATE KEY UPDATE ultima_sesion=NOW()");
        $stmt->bind_param("s", $uid);
        $stmt->execute();
        echo " | DB: Registro OK";
        exit;
    }

    if (isset($_POST['guardar_puntos'])) {
        $uid = $_POST['uid'];
        $puntos = intval($_POST['puntos']);
        
        $stmt1 = $conn->prepare("INSERT INTO partidas (uid_hex, puntos_ganados) VALUES (?, ?)");
        $stmt1->bind_param("si", $uid, $puntos);
        if ($stmt1->execute()) {
            $stmt2 = $conn->prepare("UPDATE jugadores SET puntos_totales = puntos_totales + ? WHERE uid_hex = ?");
            $stmt2->bind_param("is", $puntos, $uid);
            $stmt2->execute();
            echo "SUCCESS: Puntos ($puntos) guardados para $uid";
        } else {
            echo "ERROR: No se pudo guardar la partida";
        }
        exit;
    }

    $conn->close();

} catch (Exception $e) {
    // Si la BD falla, al menos ya escribimos el archivo para que el juego inicie
    echo " | AVISO DB: " . $e->getMessage();
}
?>