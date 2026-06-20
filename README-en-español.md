**Un plugin para OBS que mejora el flujo de edición tanto en preproducción como en postproducción. Ofrece un dock unificado con acceso rápido a propiedades de fuentes, filtros de fuente y escena, transformaciones y otras herramientas para agilizar la edición de escenas.**

Este dock replantea la interfaz de OBS para que no tengas que estar abriendo y cerrando menús y ventanas constantemente, ahorrando tiempo en el desarrollo de tus proyectos.

El acceso a los paneles se realiza a través de los botones de la cabecera que están marcados con un borde de color verde. El botón que representa al panel actual se marca con el borde en color rojo. También en la cabecera puedes editar el nombre de la escena y fuente seleccionadas.

(El plugin está traducido a todos los idiomas posibles en OBS)

---

## 🟢 PANEL DE FUENTE
Dependiendo de la fuente seleccionada, el panel muestra u oculta automáticamente las acciones disponibles.

### Acciones comunes para todas las fuentes
- Copiar, pegar referencia y pegar duplicado
- Etiquetas de color para la fuente seleccionada

### Acciones específicas de imagen
- Captura de pantalla
- Abrir proyector de la fuente
- Filtro de escala
- Modo de fusión
- Método de fusión

### Acciones específicas de vídeo
- Desentrelazado

### Transiciones de fuente al mostrar y ocultar
- Propiedades de transición
- Copiar y pegar transición

### Acciones específicas de audio
- Modo mono / estéreo
- Control de balance
- Intervalo de sincronización
- Opciones de monitorización de audio
- Selección de pistas de audio

![Panel de fuente](images-readme/source.png)

---

## 🟢 PANEL DE PROPIEDADES
El panel de propiedades muestra exactamente las mismas opciones que ofrece OBS, pero integradas dentro del dock para evitar abrir ventanas flotantes.  
Las propiedades se actualizan automáticamente al cambiar de fuente, sin necesidad de cerrar ni volver a abrir nada.

Este panel respeta el diseño original de OBS, incluyendo:
- Grupos de propiedades
- Controles personalizados
- Propiedades dinámicas según el tipo de fuente
- Actualización en tiempo real al modificar valores

### Características destacadas
- Las propiedades se muestran siempre dentro del dock, sin ventanas externas.
- Cambiar de fuente actualiza el panel al instante.
- Compatible con todas las fuentes estándar de OBS.
- Compatible con propiedades personalizadas de plugins externos.

![Propiedades 1](images-readme/properties1.png)
![Propiedades 2](images-readme/properties2.png)
![Propiedades 3](images-readme/properties3.png)

---

## 🟢 PANEL DE TRANSFORMACIÓN
El panel de transformación permite modificar la posición, escala, rotación, alineación y recorte de la fuente seleccionada, todo desde el dock y sin abrir el menú ni el panel nativo de OBS.

El objetivo es ofrecer un control rápido y centralizado de los ajustes más utilizados durante la edición de escenas.

### Controles disponibles
- Copiar, pegar y restablecer transformación
- Voltear horizontal y vertical
- Girar 90º a derecha e izquierda, y girar 180º
- Centrar vertical y horizontal
- Ajustar a pantalla
- Estirar a pantalla
- Centrar en pantalla
- Posición X / Y
- Rotación  
- Escala X / Y  
- Punto de anclaje  
- Alineación  
- Tamaño de bounding box  
- Tipo de bounding box (None, Stretch, Scale Inner, Scale Outer)  
- Recorte (izquierda, derecha, arriba, abajo)

### Características destacadas
- Actualización automática al cambiar de fuente  
- Sin ventanas emergentes  
- Valores sincronizados en tiempo real con OBS  
- Compatible con todas las fuentes que soportan transformaciones  

![Transformación 1](images-readme/transform.png)

---

## 🟢 PANELES DE FILTROS
El dock incluye dos paneles independientes: **Filtros de fuente** y **Filtros de escena**.  
Ambos funcionan igual que los filtros nativos de OBS, pero integrados dentro del dock y con mejoras importantes para la edición rápida.

Estos paneles cuentan con una cabecera propia para el control preciso de la edición de filtros.
- Icono para expandir o colapsar todas las propiedades de los filtros.
- Iconos para mostrar u ocultar los filtros.
- Un botón para agregar filtros.
- Un botón para eliminar los filtros seleccionados.
- Dos botones para intercambiar entre filtros de **EFECTO** y filtros de **AUDIO**. Si la fuente seleccionada no permite filtros de audio el botón queda apagado e inútil.
- Botones para copiar y pegar los filtros seleccionados.
- Checkbox para seleccionar o deseleccionar todos los filtros.
- Botón para abrir todos los filtros de la lista en ventanas independientes.

Los filtros se muestran en una lista con:
- Icono de expandir/colapsar
- Icono de visibilidad
- Nombre del filtro (editable)
- Iconos para mover arriba y abajo
- Checkbox de selección
- Botón para abrir el filtro en una ventana externa (multiedición)

Al expandir un filtro, sus propiedades aparecen directamente debajo, empujando el resto de la lista de los filtros hacia abajo.

### Características destacadas
- Lista de filtros siempre visible dentro del dock  
- Propiedades integradas sin abrir ventanas emergentes  
- Renombrado directo del filtro  
- Visibilidad sincronizada con OBS  
- Botón para abrir filtros en ventanas independientes 
- Actualización automática al cambiar de fuente o escena  

![Filtros 1](images-readme/effect-filters.png)
![Filtros 2](images-readme/audio-filters.png)
![Filtros 3](images-readme/filters-popout.png)

---

## 🎨 ICONOS PERSONALIZADOS 

He diseñado y añadido iconos para:
- Panel de transformación
- Copiar
- Pegar
- Restablecer
- Voltear horizontal
- Voltear vertical
- Girar 90º derecha
- Girar 90º izquierda
- Girar 180º

## ⚠️ ADVERTENCIA ⚠️

**⚠️⚠️⚠️⚠️Todos los iconos funcionan perfectos en el tema Yami de OBS.  
Si usas otro tema los iconos no se mostrarán.⚠️⚠️⚠️⚠️**

---

## 📜 NOTA PERSONAL
Tengo que confesar que **no he escrito ni una sola línea de código** porque soy un ignorante sobre temas de programación, así que para el desarrollo de este plugin me he apoyado en <img src="images-readme/copilot.png" height="18">, yo solo he tratado de guiar el proceso lo mejor que he sabido.

Este plugin es y será siempre **GRATUITO**. Es mi forma de agradecer a la comunidad sus aportaciones de las que tanto he aprendido en mis proyectos.

**¡¡ARRIBA ESPAÑA!!**
**¡¡VIVA CRISTO REY!!**