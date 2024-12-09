Integrantes del equipo:
    Leticia Bargiela de Jesus
    Juana Maria Luna Carvajal
    Serafí Nebot Ginard

Mejoras notables
- El uso de macros para imprimir mensajes de error y depuración. De esta forma no es necesario copiar y pegar el mismo código para cambiar el color del texto o comprobar si DEBUGNx esta habilitado. Sólo se define en un único sitio.
- Se ha añadido una variable global, cmdwait, para asegurar-se de que las funciones reaper/ctrlc/ctrlz impriman texto en la linea que toca y vuelvan a imprimir el prompt si es necesario.
- Tabla de comandos internos que asocia el nombre del comando con la función que debe ejecutar. De esta forma evitamos concatenar múltiples ifs y el código queda más simple y limpio.
