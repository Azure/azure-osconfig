#[derive(Clone, Copy)]
pub struct Sample {
    max_payload_size_bytes: u32
}

// impl std::clone::Clone for Number {
//     fn clone(&self) -> Self {
//         Self { ..*self }
//     }
// }

// impl Number {
//     fn is_strictly_positive(self) -> bool {
//         self.value > 0
//     }
// }