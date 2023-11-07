import tensorflow as tf

layer = tf.keras.layers
seq = tf.keras.models.Sequential

def vgg19():
    model = seq([
        # Block 1
        layer.Conv2D(64, kernel_size=(3, 3), padding='same', activation='relu', input_shape=(224, 224, 3)),
        layer.Conv2D(64, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.MaxPooling2D(pool_size=(2, 2), strides=(2, 2)),

        # Block 2
        layer.Conv2D(128, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.Conv2D(128, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.MaxPooling2D(pool_size=(2, 2), strides=(2, 2)),

        # Block 3
        layer.Conv2D(256, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.Conv2D(256, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.Conv2D(256, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.Conv2D(256, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.MaxPooling2D(pool_size=(2, 2), strides=(2, 2)),

        # Block 4
        layer.Conv2D(512, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.Conv2D(512, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.Conv2D(512, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.Conv2D(512, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.MaxPooling2D(pool_size=(2, 2), strides=(2, 2)),

        # Block 5
        layer.Conv2D(512, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.Conv2D(512, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.Conv2D(512, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.Conv2D(512, kernel_size=(3, 3), padding='same', activation='relu'),
        layer.MaxPooling2D(pool_size=(2, 2), strides=(2, 2)),

        # Fully Connected Layers
        layer.Flatten(),
        layer.Dense(4096, activation='relu'),
        layer.Dropout(0.5),
        layer.Dense(4096, activation='relu'),
        layer.Dropout(0.5),
        layer.Dense(1000, activation='softmax')  # Assuming 1000 classes

    ])

    model.compile(optimizer='adam',
                  loss='categorical_crossentropy',
                  metrics=['accuracy'])
    return model

