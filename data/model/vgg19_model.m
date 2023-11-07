Network vgg19 {
Layer block1_conv1 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 64, C: 3, R: 3, S: 3, Y: 224, X: 224 }
}
Layer block1_conv2 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 64, C: 64, R: 3, S: 3, Y: 224, X: 224 }
}
Layer block2_conv1 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 128, C: 64, R: 3, S: 3, Y: 112, X: 112 }
}
Layer block2_conv2 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 128, C: 128, R: 3, S: 3, Y: 112, X: 112 }
}
Layer block3_conv1 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 256, C: 128, R: 3, S: 3, Y: 56, X: 56 }
}
Layer block3_conv2 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 256, C: 256, R: 3, S: 3, Y: 56, X: 56 }
}
Layer block3_conv3 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 256, C: 256, R: 3, S: 3, Y: 56, X: 56 }
}
Layer block3_conv4 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 256, C: 256, R: 3, S: 3, Y: 56, X: 56 }
}
Layer block4_conv1 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 512, C: 256, R: 3, S: 3, Y: 28, X: 28 }
}
Layer block4_conv2 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 512, C: 512, R: 3, S: 3, Y: 28, X: 28 }
}
Layer block4_conv3 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 512, C: 512, R: 3, S: 3, Y: 28, X: 28 }
}
Layer block4_conv4 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 512, C: 512, R: 3, S: 3, Y: 28, X: 28 }
}
Layer block5_conv1 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 512, C: 512, R: 3, S: 3, Y: 14, X: 14 }
}
Layer block5_conv2 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 512, C: 512, R: 3, S: 3, Y: 14, X: 14 }
}
Layer block5_conv3 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 512, C: 512, R: 3, S: 3, Y: 14, X: 14 }
}
Layer block5_conv4 {
Type: CONV
Stride { X: 1, Y: 1 }
Dimensions { K: 512, C: 512, R: 3, S: 3, Y: 14, X: 14 }
}
Layer fc1 {
Type: CONV
Dimensions { K: 4096, C: 25088, R: 1, S: 1, Y: 1, X: 1 }
}
Layer fc2 {
Type: CONV
Dimensions { K: 4096, C: 4096, R: 1, S: 1, Y: 1, X: 1 }
}
Layer predictions {
Type: CONV
Dimensions { K: 1000, C: 4096, R: 1, S: 1, Y: 1, X: 1 }
}
}