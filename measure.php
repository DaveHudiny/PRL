<?php

shell_exec("mpic++ --prefix /usr/local/share/OpenMPI -o parkmeans parkmeans.cc");
$fp = fopen("results.txt", "w");
fwrite($fp, "Index ");
for($j = 1; $j <= 30; $j++)
{
    fwrite($fp, "$j ");
}
for($i = 4; $i <= 32; $i++)
{

    fwrite($fp, "{$i} ");
    for($j = 0; $j < 30; $j++)
    {
        shell_exec("dd if=/dev/random bs=1 count=$i of=numbers");
        $start = microtime(true);
        shell_exec("mpirun --prefix /usr/local/share/OpenMPI -oversubscribe -np {$i} parkmeans");
        $time_elapsed_secs = microtime(true) - $start;
        fwrite($fp, "{$time_elapsed_secs} ");
    }
    fwrite($fp, "\n");
   

}
fclose($fp);

?>