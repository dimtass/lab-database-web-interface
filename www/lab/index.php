<!--
    Main web interface file. There is a blog on how to use this
    in here: https://www.stupid-projects.com/component-database-with-led-indicators/

    Author: Dimitris Tassopoulos <dimtass@gmail.com>
-->
<html>
<head>
    <meta charset="utf-8">
    <link rel="icon" type="image/png" href="images/icon.png">
    <title>Λίστα υλικών</title>
    <link rel="stylesheet" href="css/style.css">
    <link rel="stylesheet" href="css/jquery.fancybox.min.css" type="text/css" media="screen" />
    <script type="text/javascript" src="js/jquery-3.4.1.min.js"></script>
    <script type="text/javascript" src="js/jquery.fancybox.min.js"></script>

    <script type="text/javascript">
    var esp8266ip = "";
    $(document).ready(function() {
        <?php
            $file = fopen("ESP8266IP.txt", "r") or die('Unable to open the ESP8266IP.txt file');
            $ip = fgets($file);
            echo 'esp8266ip = "' . $ip . '";';
            fclose($file);
        ?>

        $('[data-fancybox="gallery"]').fancybox({
            // Options will go here
            modal: false
        });

        // detect if ESP8266 exists
        $.post( "http://" + esp8266ip + "/", function( data ) {
            // parse data to find if the aREST server is running
        });

    });
    // This will turn on a single LED on
    function turn_on_led(index) {
        $.post( "http://" + esp8266ip + "/led_index?params=" + index, function( data ) {
            $(".rest_reply" ).html( data );
        });
    }
    // This will set the ambient color
    function ambient_color() {
        var color = parseInt('0x' + document.getElementById("ambient_color").value.replace('#', ''), 16);
        // alert('color: ' + color);
        $.post( "http://" + esp8266ip + "/led_ambient?params=" + color, function( data ) {
            $(".rest_reply" ).html( data );
        });
    }
    // This will save the ambient color
    function save_ambient_color() {
        var color = parseInt('0x' + document.getElementById("ambient_color").value.replace('#', ''), 16);
        // alert('color: ' + color);
        $.post( "http://" + esp8266ip + "/save_led_ambient?params=" + color, function( data ) {
            $(".rest_reply" ).html( data );
        });
    }
    // This will turn on/off the ambient color
    function ambient_light(onoff) {
        $.post( "http://" + esp8266ip + "/enable_ambient?params=" + onoff, function( data ) {
            $(".rest_reply" ).html( data );
        });
    }
    </script>
</head>
<body>

<div>
<br><br>
<p id="search">Search part:</p>
<form method="post" action="index.php" id="searchform">
    <input type="text" name="part">
    <input type="submit" name="submit" value="Search">
</form>
<br>
<p id="extra_functions">
    Extra functions:<br>
    <button onclick="window.location.href = './upload.html';">Add new part</button>
    <button onclick="ambient_light(1)">Enable ambient</button>
    <button onclick="ambient_light(0)">Disable ambient</button>
    <input oninput="ambient_color()" onchange="save_ambient_color()" type="color" id="ambient_color" name="ambient_color" value="#ffffff">
</p>
</div>
<div id="div_parts_list">
    <ul id="parts">
        <?php
            // Create a db instance
            class MyDB extends SQLite3
            {
                function __construct()
                {
                    $this->open('parts.db');
                }
            }
            $db = new MyDB();
            if(!$db){
                echo 'Failed to open database!!!'.'<br>';
                echo $db->lastErrorMsg();
            }
            // by default list all the parts in the database
            $sql = "SELECT * from tbl_parts";
            // check if a submit is done
            if(isset($_POST['submit'])){ 
                // check for valid search string
                $part=$_POST['part'];	// get part
                $sql = 'SELECT * from tbl_parts WHERE Part LIKE "%'.$part.'%" OR Description LIKE "%'.$part.'%" OR Tags LIKE "%'.$part.'%"';
            }
            echo '<br>Results for query: '.$sql.'<br><br>';

            $ret = $db->query($sql);
            while($row = $ret->fetchArray(SQLITE3_ASSOC)) {		// search for image
                $img_name = "no_image.png";
                if (!empty($row['Image'])) {
                    $img_name = $row['Image'];
                    $img_caption = $row['Part'];
                }
                echo '<li><a data-fancybox="gallery" data-caption="'.$img_caption.'" href="./images/'.$img_name.'"><img src="./thumbs/'.$img_name.'" alt="" /></a>';
                echo '<h2>'.$row['Part'].'</h2>';
                echo '<p>'.$row['Description'].'</p></a>';
                if (!empty($row['Datasheet'])) {
                    echo '<a href="./datasheets/'.$row['Datasheet'].'"><img class="pdf" src="./images/pdf.png"/></a>';
                }
                if (!empty($row['Location']) && $ip) {
                    echo '<button onclick="turn_on_led(' . $row['Location'] . ')">Show</button>';
                }
                echo '</li>';
            }
            echo '<br><br>';
            $db->close();
		?>
	</ul>
</div>
Author: Dimitris Tassopoulos - dimtass@gmail.com - 2014-2020
</body>
</head>
