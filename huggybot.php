<?php
$arena=$argv[1];
list($meX, $meY)=findMe($arena);
list($oppX, $oppY)=findOpp($arena);
if($meY<$oppY)
{
    if($meX<$oppX)
        echo "SE";
    elseif($meX==$oppX)
        echo "S";
    else
        echo "SW";
}
elseif($meY==$oppY)
{
    if($meX<$oppX)
        echo "E";
    else
        echo "W";
}
else
{
    if($meX<$oppX)
        echo "NE";
    elseif($meX==$oppX)
        echo "N";
    else
        echo "NW";
}

function findMe($arena)
{
    return find("Y", explode("\n", $arena));
}

function findOpp($arena)
{
    return find("X", explode("\n", $arena));
}

function find($char, $array)
{
    $x=0;
    $y=0;
    for($loop=0;$loop<10;$loop++)
    {
        if(strpos($array[$loop], $char)!==FALSE)
        {
            $x=strpos($array[$loop], $char);
            $y=$loop;
        }
    }
    return array($x, $y);
}
?>