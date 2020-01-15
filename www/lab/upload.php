<!--
    Upload functionality for the main web interface. There is a blog on how to use this
    in here: https://www.stupid-projects.com/component-database-with-led-indicators/

    Author: Dimitris Tassopoulos <dimtass@gmail.com>
-->

<?php
ini_set('display_errors', 1);
require "CreateThumbnail.php";

$images_dir = 'images/';
$datasheets_dir = 'datasheets/';
$thumbs_dir = 'thumbs/';
// Thumbs size
$thumbs_width = '320';
$thumbs_height = '320';
// Get parameters
$partName = $_POST['partName'];
$partDescription = $_POST['partDescription'];
$partDatasheet = basename($_FILES["partDatasheet"]["name"]);
$partImage = basename($_FILES["partImage"]["name"]);
$partManufacturer = $_POST['partManufacturer'];
$partQty = $_POST['partQty'];
$partTags = $_POST['partTags'];
$partLocation = $_POST['partLocation'];
// Get files
$target_file_image = $images_dir . $partImage;
$target_thumb_image = $thumbs_dir . $partImage;
$target_file_datasheet = $datasheets_dir . $partDatasheet;

echo "Part name: " . $partName . "<br>";
echo "Image upload: " . $target_file_image . "<br>";
echo "Datasheet upload: " . $target_file_datasheet . "<br>";

// Check if image file is a actual image or fake image
if(isset($_POST["submit"])) {
    if (!$partName) {
        echo "[Error]: You need to set the part name...<br>";
        return;
    }
    // Check datasheet
    if ($partDatasheet) {
        if (file_exists($target_file_datasheet)) {
            echo "[Error]: File already exists -> " . $target_file_datasheet . "<br>";
            return;
        }
    }
    // Check image file
    if ($partImage) {
        $check = getimagesize($_FILES["partImage"]["tmp_name"]);
        if($check == false) {
            echo "[Error]: File is not an image.<br>";
            return;
        }
        // Check if file already exists
        if (file_exists($target_file_image)) {
            echo "[Error]: File already exists -> " . $target_file_image . "<br>";
            return;
        }
    }
}

// Add part to database
class MyDB extends SQLite3
{
    function __construct()
    {
        $this->open('./parts.db');
    }
}
// Open database
$db = new MyDB();
if(!$db){
    echo 'Failed to open database!!!'.'<br>';
    echo $db->lastErrorMsg();
}
// Insert new record
// $sql = "INSERT INTO tbl_parts ('Part')
//         VALUES ('". $partName . "');";
$sql = "INSERT INTO tbl_parts (Part,Description,Datasheet,Image,Manufacturer,Qty,Tags,Location)
        VALUES ('". $partName . "','" .
        $partDescription . "','" .
        $partDatasheet . "','" .
        $partImage . "','" .
        $partManufacturer . "','" .
        $partQty . "','" .
        $partTags . "','" .
        $partLocation . "');";
echo "SQL: " . $sql . "<br>";
$ret = $db->exec($sql);
echo "ret: " . $ret . "<br>";
$db->close();

// Add image
if ($partImage) {
    if (move_uploaded_file($_FILES["partImage"]["tmp_name"], $target_file_image)) {
        echo "The file ". basename( $_FILES["partImage"]["name"]). " has been uploaded.<br>";
        // Run the python script that handles the files
    } else {
        echo "[Error]: There was an error uploading your file.";
        return;
    }
    // Resize image to thumbnail size and place it in the thumbs/ folder
    createThumbnail($partImage, $thumbs_width, $thumbs_height, $images_dir, $thumbs_dir);
}
if ($partDatasheet) {
    // Add datasheet
    if (move_uploaded_file($_FILES["partDatasheet"]["tmp_name"], $target_file_datasheet)) {
        echo "The file ". basename( $_FILES["partDatasheet"]["name"]). " has been uploaded.";
        echo '<p><button onclick="window.location.href = \'./index.php\';">Return</button></p>';
    } else {
        echo "[Error]: There was an error uploading your file.";
        return;
    }
}
?>