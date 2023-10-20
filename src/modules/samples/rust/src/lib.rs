// Copyright (c) Microsoft Corporation. All rights reserved..
// Licensed under the MIT License.

use osc::osc_module;
use serde::{Serialize, Deserialize};

#[derive(Default)]
struct Sample {
    x: i32,
}

#[osc_module]
impl Sample {
    #[osc(reported)]
    fn simple(&self) -> i32 {
       self.x
    }

    #[osc(desired)]
    fn desired_simple(&mut self, x: i32) {
        self.x = x;
    }

    #[osc(reported)]
    fn complex() -> Vec<Complex> {
        vec![
            Complex {
                x: 42,
                y: "hello".to_string(),
            },
            Complex {
                x: 43,
                y: "world".to_string(),
            },
        ]
    }

    #[osc(desired)]
    fn desired_complex(x: Vec<Complex>) {
        println!("desired_complex: {:?}", x);
    }
}

#[derive(Debug, Serialize, Deserialize, Default)]
struct Complex {
    x: i32,
    y: String,
}
