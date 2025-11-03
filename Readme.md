Proyecto3IA — Motor de inferencia por enumeración (Redes Bayesianas)

# 🎓 Proyecto 3 — Motor de Inferencia por Enumeración (Redes Bayesianas)

> 🧠 *Implementación en C++ orientado a objetos para realizar inferencias exactas sobre Redes Bayesianas mediante enumeración.*

---

## 🧾 Descripción

Este proyecto implementa un **motor de inferencia por enumeración** para **Redes Bayesianas**, con un enfoque modular y orientado a objetos.  
Permite:

✅ Cargar la estructura de una red desde un archivo (`estructura.txt`).  
✅ Cargar tablas de probabilidad condicional (CPTs) desde un archivo (`cpts.txt`).  
✅ Mostrar la estructura y las CPTs por consola.  
✅ Ejecutar **consultas de probabilidad condicional** mediante **enumeración exacta**.  
✅ (Opcional) Generar una **traza paso a paso** del proceso de inferencia, útil para depuración o docencia.

---

## 🧩 Estructura del código

| Archivo | Descripción |
|----------|-------------|
| `main.cpp` | Interfaz de línea de comandos y parsing de comandos. |
| `red_bayesiana.*` | Representación del grafo y carga de la red. |
| `tabla_probabilidad.*` | Gestión e impresión de las tablas de probabilidad condicional. |
| `inferencia.*` | Motor de inferencia por enumeración exacta. |
| `nodo.*` | Clase para cada nodo (variable aleatoria) de la red. |
| `util.*` | Funciones auxiliares: parsing, trimming, empaquetado de claves. |

---

## ⚙️ Requisitos

- **C++17** o superior (`g++ 7+` recomendado)  
- En Windows: **MinGW-w64** o **MSYS2**, o usa **WSL** con GCC  
- (Opcional) **AddressSanitizer (ASAN)** y **GDB** para depuración

---

## 🛠️ Compilación

### 🔹 Rápida (Linux / WSL)

```bash
g++ -std=c++17 -O2 -Wall -Wextra src/*.cpp -Iinclude -o bn
```

### 🔹 Modo depuración

```bash
g++ -std=c++17 -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer src/*.cpp -Iinclude -o bn_asan
```

### 🔹 En Windows (PowerShell)

```powershell
g++ -std=c++17 -O2 -Wall -Wextra src/*.cpp -Iinclude -o bn.exe
```

## 📂 Archivos de entrada

### 🗺️ `estructura.txt`
Cada línea no vacía (ni comentada con `#`) describe una relación **Padre → Hijo**:

```text
# Ejemplo de estructura
Lluvia -> Mantenimiento
Lluvia -> Tren
Mantenimiento -> Tren
Tren -> Cita
```

---

### 📊 `cpts.txt`

Define las **tablas de probabilidad condicional (CPTs)**.  
Cada bloque representa un nodo:

```text
NODE <Nombre>
VALUES: <valor1> <valor2> ...
PARENTS: <Padre1> <Padre2> ...    # opcional
TABLE
<filas de probabilidades>
END
```

#### 🔸 Ejemplo:
```text
NODE Lluvia
VALUES: ninguna ligera fuerte
TABLE
p: 0.7 0.2 0.1
END
```

```text
NODE Cita
VALUES: asiste falta
PARENTS: Tren
TABLE
Tren=a_tiempo : 0.9 0.1
Tren=retrasado: 0.6 0.4
END
```

---

## 💻 Uso

El programa recibe como mínimo los dos archivos de entrada:

```bash
./bn estructura.txt cpts.txt [COMANDO]
```

### 🔹 Comandos disponibles:

| Comando | Descripción |
|----------|--------------|
| `MOSTRAR:ESTRUCT` | Muestra la estructura de la red (orden topológico y padres). |
| `MOSTRAR:CPTS` | Imprime todas las tablas de probabilidad (CPTs). |
| `CONSULTAR: <Var> | <EVIDENCIA>` | Ejecuta una inferencia exacta. Ejemplo:<br>`CONSULTAR: Cita | Tren=a_tiempo` |
| `CONSULTAR_TRACE: <Var> | <EVIDENCIA>` | Igual que `CONSULTAR`, pero mostrando paso a paso la enumeración. |

---

## ⚡ Ejemplos rápidos

```bash
# Mostrar estructura
./bn estructura.txt cpts.txt MOSTRAR:ESTRUCT

# Mostrar tablas de probabilidad
./bn estructura.txt cpts.txt MOSTRAR:CPTS

# Consulta simple
./bn estructura.txt cpts.txt 'CONSULTAR: Cita | Tren=a_tiempo'

# Consulta con evidencia múltiple
./bn estructura.txt cpts.txt 'CONSULTAR: Cita | Tren=retrasado, Mantenimiento=no, Lluvia=ligera'

# Consulta con traza detallada
./bn estructura.txt cpts.txt 'CONSULTAR_TRACE: Cita | Tren=retrasado, Mantenimiento=no, Lluvia=ligera'
```

---

## 🧠 Ejemplo de inferencia

📍 *Probabilidad de faltar a la reunión si el tren está retrasado, no hay mantenimiento y llueve ligeramente:*

\[
P(Cita = falta \mid Tren = retrasado, Mantenimiento = no, Lluvia = ligera) = 0.4
\]

---

## 🔍 Depuración

Compila con sanitizadores para detectar errores de memoria:

```bash
g++ -std=c++17 -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer src/*.cpp -Iinclude -o bn_asan
```

Si ocurre un *segmentation fault*, usa `gdb bn_asan` para inspeccionar la traza.

---

## 🧭 Buenas prácticas

- Usa comillas simples `' '` si la consulta contiene espacios o `|`.  
- Asegúrate de que los archivos `.txt` no contengan errores de formato (falta de `:`, `=`, o `END`).  
- Ejecuta `MOSTRAR:ESTRUCT` antes de consultar, para validar la red.  

---

## 📜 Licencia y créditos

Este proyecto fue desarrollado con fines académicos.  
Puedes modificarlo, redistribuirlo y adaptarlo libremente con fines educativos o experimentales.


- Si `TABLE` contiene una línea que empieza con `p:` se interpreta como la fila prior (sin condicionantes): `p: 0.7 0.2 0.1`.
- Si la tabla tiene padres, cada fila tiene la forma `Padre1=val, Padre2=val : p1 p2 ...`.

Ejemplo (incluido en el repo): `cpts.txt` contiene los bloques para `Lluvia`, `Mantenimiento`, `Tren` y `Cita`.

Uso: comandos principales
------------------------

El ejecutable recibe al menos dos argumentos: `estructura.txt` y `cpts.txt`. Opcionalmente puedes pasar comandos adicionales que el programa ejecutará en orden:

- MOSTRAR:ESTRUCT
	- Imprime la estructura (orden topológico) y padres de cada nodo.

- MOSTRAR:CPTS
	- Imprime las tablas de probabilidad (CPTs) para cada variable.

- CONSULTAR: <Var> | <EVIDENCIA>
	- Ejecuta una consulta por enumeración. `EVIDENCIA` es una lista separada por comas de asignaciones `Var=valor`.
	- Ejemplo:
		- ./bn estructura.txt cpts.txt 'CONSULTAR: Cita | Tren=a_tiempo'

- CONSULTAR_TRACE: <Var> | <EVIDENCIA>
	- Igual que `CONSULTAR` pero además imprime una traza paso a paso (pensada para depuración y aprendizaje).
	- Ejemplo:
		- ./bn estructura.txt cpts.txt 'CONSULTAR_TRACE: Cita | Tren=a_tiempo'

Ejemplos rápidos
----------------

- Mostrar estructura:
	./bn estructura.txt cpts.txt MOSTRAR:ESTRUCT

- Mostrar CPTs:
	./bn estructura.txt cpts.txt MOSTRAR:CPTS

- Consulta simple:
	./bn estructura.txt cpts.txt 'CONSULTAR: Cita | Tren=a_tiempo'

- Consulta con evidencia múltiple:
	./bn estructura.txt cpts.txt 'CONSULTAR: Cita | Tren=retrasado, Mantenimiento=no, Lluvia=ligera'

- Consulta con traza (útil para ver cómo se calcula la probabilidad):
	./bn estructura.txt cpts.txt 'CONSULTAR_TRACE: Cita | Tren=retrasado, Mantenimiento=no, Lluvia=ligera'


Depuración y comprobaciones
---------------------------

- Compila con ASAN si sospechas problemas de memoria:
	g++ -std=c++17 -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer ... -o bn_asan

- Si obtienes un segmentation fault, ejecuta con `bn_asan` o instala `gdb` en WSL y genera un backtrace.

Buenas prácticas
---------------

- Usa comillas simples alrededor del comando de consulta si contiene espacios o caracteres especiales (barra vertical `|`).
- Valida que `estructura.txt` y `cpts.txt` no contengan líneas con errores de sintaxis (falta `:`, `=` mal puesto, etc.).


Contacto / Licencia
-------------------

Este proyecto es un ejercicio académico. Adáptalo libremente para tus propósitos.
