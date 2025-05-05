{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 9,
			"minor" : 0,
			"revision" : 5,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"classnamespace" : "box",
		"rect" : [ 34.0, 100.0, 1157.0, 848.0 ],
		"gridsize" : [ 15.0, 15.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"autosave" : 1,
					"id" : "obj-4",
					"inletInfo" : 					{
						"IOInfo" : [ 							{
								"type" : "signal",
								"index" : 1,
								"tag" : "in1",
								"comment" : ""
							}
 ]
					}
,
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outletInfo" : 					{
						"IOInfo" : [ 							{
								"type" : "signal",
								"index" : 1,
								"tag" : "out1",
								"comment" : ""
							}
 ]
					}
,
					"outlettype" : [ "signal", "list" ],
					"patching_rect" : [ 815.0, 721.0, 113.0, 22.0 ],
					"rnboattrcache" : 					{
						"phase" : 						{
							"label" : "phase",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"cutoff" : 						{
							"label" : "cutoff",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"q" : 						{
							"label" : "q",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"pitch" : 						{
							"label" : "pitch",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"mix" : 						{
							"label" : "mix",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"depth" : 						{
							"label" : "depth",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"rate" : 						{
							"label" : "rate",
							"isEnum" : 0,
							"parsestring" : ""
						}

					}
,
					"rnboversion" : "1.3.4",
					"saved_attribute_attributes" : 					{
						"valueof" : 						{
							"parameter_invisible" : 1,
							"parameter_longname" : "rnbo~",
							"parameter_modmode" : 0,
							"parameter_shortname" : "rnbo~",
							"parameter_type" : 3
						}

					}
,
					"saved_object_attributes" : 					{
						"optimization" : "O1",
						"parameter_enable" : 1,
						"uuid" : "ca9bdcf7-2973-11f0-9cf7-2e5648a92596"
					}
,
					"snapshot" : 					{
						"filetype" : "C74Snapshot",
						"version" : 2,
						"minorversion" : 0,
						"name" : "snapshotlist",
						"origin" : "rnbo~",
						"type" : "list",
						"subtype" : "Undefined",
						"embed" : 1,
						"snapshot" : 						{
							"phase" : 							{
								"value" : 1.0
							}
,
							"rate" : 							{
								"value" : 0.2
							}
,
							"mix" : 							{
								"value" : 0.5
							}
,
							"q" : 							{
								"value" : 0.5
							}
,
							"cutoff" : 							{
								"value" : 0.3
							}
,
							"depth" : 							{
								"value" : 0.5
							}
,
							"pitch" : 							{
								"value" : 0.484615384615384
							}
,
							"__presetid" : "mono-chorus"
						}
,
						"snapshotlist" : 						{
							"current_snapshot" : 0,
							"entries" : [ 								{
									"filetype" : "C74Snapshot",
									"version" : 2,
									"minorversion" : 0,
									"name" : "mono-chorus",
									"origin" : "mono-chorus",
									"type" : "rnbo",
									"subtype" : "",
									"embed" : 0,
									"snapshot" : 									{
										"phase" : 										{
											"value" : 1.0
										}
,
										"rate" : 										{
											"value" : 0.2
										}
,
										"mix" : 										{
											"value" : 0.5
										}
,
										"q" : 										{
											"value" : 0.5
										}
,
										"cutoff" : 										{
											"value" : 0.3
										}
,
										"depth" : 										{
											"value" : 0.5
										}
,
										"pitch" : 										{
											"value" : 0.484615384615384
										}
,
										"__presetid" : "mono-chorus"
									}
,
									"fileref" : 									{
										"name" : "mono-chorus",
										"filename" : "mono-chorus.maxsnap",
										"filepath" : "~/Documents/Max 9/Snapshots",
										"filepos" : -1,
										"snapshotfileid" : "d7b7eeed850f75a4d799ce8ec3afce3d"
									}

								}
 ]
						}

					}
,
					"text" : "rnbo~ mono-chorus",
					"varname" : "rnbo~"
				}

			}
, 			{
				"box" : 				{
					"floatoutput" : 1,
					"id" : "obj-3",
					"maxclass" : "slider",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 870.0, 382.0, 20.0, 140.0 ],
					"size" : 3.0
				}

			}
, 			{
				"box" : 				{
					"attr" : "pitch",
					"id" : "obj-1",
					"maxclass" : "attrui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 829.0, 618.0, 150.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-5",
					"maxclass" : "meter~",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "float" ],
					"patching_rect" : [ 638.0, 754.0, 80.0, 13.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-32",
					"maxclass" : "meter~",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "float" ],
					"patching_rect" : [ 475.0, 758.0, 80.0, 13.0 ]
				}

			}
, 			{
				"box" : 				{
					"autosave" : 1,
					"id" : "obj-31",
					"inletInfo" : 					{
						"IOInfo" : [ 							{
								"type" : "signal",
								"index" : 1,
								"tag" : "in1",
								"comment" : ""
							}
, 							{
								"type" : "signal",
								"index" : 2,
								"tag" : "in2",
								"comment" : ""
							}
 ]
					}
,
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 3,
					"outletInfo" : 					{
						"IOInfo" : [ 							{
								"type" : "signal",
								"index" : 1,
								"tag" : "out1",
								"comment" : ""
							}
, 							{
								"type" : "signal",
								"index" : 2,
								"tag" : "out2",
								"comment" : ""
							}
 ]
					}
,
					"outlettype" : [ "signal", "signal", "list" ],
					"patching_rect" : [ 589.0, 666.0, 120.0, 22.0 ],
					"rnboattrcache" : 					{
						"cutoff" : 						{
							"label" : "cutoff",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"left/depth" : 						{
							"label" : "depth",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"left/q" : 						{
							"label" : "q",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"right/pitch" : 						{
							"label" : "pitch",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"q" : 						{
							"label" : "q",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"pitch" : 						{
							"label" : "pitch",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"right/cutoff" : 						{
							"label" : "cutoff",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"mix" : 						{
							"label" : "mix",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"left/rate" : 						{
							"label" : "rate",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"right/depth" : 						{
							"label" : "depth",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"right/mix" : 						{
							"label" : "mix",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"depth" : 						{
							"label" : "depth",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"right/phase" : 						{
							"label" : "phase",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"left/pitch" : 						{
							"label" : "pitch",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"rate" : 						{
							"label" : "rate",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"left/cutoff" : 						{
							"label" : "cutoff",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"right/q" : 						{
							"label" : "q",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"left/phase" : 						{
							"label" : "phase",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"right/rate" : 						{
							"label" : "rate",
							"isEnum" : 0,
							"parsestring" : ""
						}
,
						"left/mix" : 						{
							"label" : "mix",
							"isEnum" : 0,
							"parsestring" : ""
						}

					}
,
					"rnboversion" : "1.3.4",
					"saved_attribute_attributes" : 					{
						"valueof" : 						{
							"parameter_initial" : [ 								{
									"filetype" : "C74Snapshot",
									"version" : 2,
									"minorversion" : 0,
									"name" : "stereo-chorus",
									"origin" : "stereo-chorus",
									"type" : "rnbo",
									"subtype" : "",
									"embed" : 1,
									"snapshot" : 									{
										"__sps" : 										{
											"right" : 											{
												"depth" : 												{
													"value" : 0.5
												}
,
												"rate" : 												{
													"value" : 0.2
												}
,
												"phase" : 												{
													"value" : 1.0
												}
,
												"q" : 												{
													"value" : 0.5
												}
,
												"mix" : 												{
													"value" : 0.5
												}
,
												"cutoff" : 												{
													"value" : 0.3
												}

											}
,
											"left" : 											{
												"depth" : 												{
													"value" : 0.5
												}
,
												"rate" : 												{
													"value" : 0.2
												}
,
												"phase" : 												{
													"value" : 1.0
												}
,
												"q" : 												{
													"value" : 0.5
												}
,
												"mix" : 												{
													"value" : 0.4
												}
,
												"cutoff" : 												{
													"value" : 0.3
												}

											}

										}
,
										"mix" : 										{
											"value" : 0.4
										}
,
										"q" : 										{
											"value" : 0.5
										}
,
										"rate" : 										{
											"value" : 0.2
										}
,
										"phase" : 										{
											"value" : 1.0
										}
,
										"depth" : 										{
											"value" : 0.5
										}
,
										"cutoff" : 										{
											"value" : 0.3
										}
,
										"__presetid" : "stereo-chorus"
									}

								}
 ],
							"parameter_initial_enable" : 1,
							"parameter_invisible" : 1,
							"parameter_longname" : "rnbo~[2]",
							"parameter_modmode" : 0,
							"parameter_shortname" : "rnbo~[2]",
							"parameter_type" : 3
						}

					}
,
					"saved_object_attributes" : 					{
						"optimization" : "O1",
						"parameter_enable" : 1,
						"uuid" : "e2defb51-2535-11f0-bb51-2e5648a92596"
					}
,
					"snapshot" : 					{
						"filetype" : "C74Snapshot",
						"version" : 2,
						"minorversion" : 0,
						"name" : "snapshotlist",
						"origin" : "rnbo~",
						"type" : "list",
						"subtype" : "Undefined",
						"embed" : 1,
						"snapshot" : 						{
							"__sps" : 							{
								"right" : 								{
									"q" : 									{
										"value" : 0.5
									}
,
									"mix" : 									{
										"value" : 0.5
									}
,
									"phase" : 									{
										"value" : 1.0
									}
,
									"rate" : 									{
										"value" : 0.2
									}
,
									"cutoff" : 									{
										"value" : 0.3
									}
,
									"depth" : 									{
										"value" : 0.5
									}
,
									"pitch" : 									{
										"value" : 0.484615384615384
									}

								}
,
								"left" : 								{
									"q" : 									{
										"value" : 0.5
									}
,
									"mix" : 									{
										"value" : 0.5
									}
,
									"phase" : 									{
										"value" : 1.0
									}
,
									"rate" : 									{
										"value" : 0.2
									}
,
									"cutoff" : 									{
										"value" : 0.3
									}
,
									"depth" : 									{
										"value" : 0.5
									}
,
									"pitch" : 									{
										"value" : 0.484615384615384
									}

								}

							}
,
							"mix" : 							{
								"value" : 0.4
							}
,
							"q" : 							{
								"value" : 0.5
							}
,
							"pitch" : 							{
								"value" : 0.484615384615384
							}
,
							"depth" : 							{
								"value" : 0.5
							}
,
							"cutoff" : 							{
								"value" : 0.3
							}
,
							"rate" : 							{
								"value" : 0.2
							}
,
							"__presetid" : "stereo-chorus"
						}
,
						"snapshotlist" : 						{
							"current_snapshot" : 0,
							"entries" : [ 								{
									"filetype" : "C74Snapshot",
									"version" : 2,
									"minorversion" : 0,
									"name" : "stereo-chorus",
									"origin" : "stereo-chorus",
									"type" : "rnbo",
									"subtype" : "",
									"embed" : 0,
									"snapshot" : 									{
										"__sps" : 										{
											"right" : 											{
												"q" : 												{
													"value" : 0.5
												}
,
												"mix" : 												{
													"value" : 0.5
												}
,
												"phase" : 												{
													"value" : 1.0
												}
,
												"rate" : 												{
													"value" : 0.2
												}
,
												"cutoff" : 												{
													"value" : 0.3
												}
,
												"depth" : 												{
													"value" : 0.5
												}
,
												"pitch" : 												{
													"value" : 0.484615384615384
												}

											}
,
											"left" : 											{
												"q" : 												{
													"value" : 0.5
												}
,
												"mix" : 												{
													"value" : 0.5
												}
,
												"phase" : 												{
													"value" : 1.0
												}
,
												"rate" : 												{
													"value" : 0.2
												}
,
												"cutoff" : 												{
													"value" : 0.3
												}
,
												"depth" : 												{
													"value" : 0.5
												}
,
												"pitch" : 												{
													"value" : 0.484615384615384
												}

											}

										}
,
										"mix" : 										{
											"value" : 0.4
										}
,
										"q" : 										{
											"value" : 0.5
										}
,
										"pitch" : 										{
											"value" : 0.484615384615384
										}
,
										"depth" : 										{
											"value" : 0.5
										}
,
										"cutoff" : 										{
											"value" : 0.3
										}
,
										"rate" : 										{
											"value" : 0.2
										}
,
										"__presetid" : "stereo-chorus"
									}
,
									"fileref" : 									{
										"name" : "stereo-chorus",
										"filename" : "stereo-chorus.maxsnap",
										"filepath" : "~/Documents/Max 9/Snapshots",
										"filepos" : -1,
										"snapshotfileid" : "a6e8eee43645541e609a5671c84bd90e"
									}

								}
 ]
						}

					}
,
					"text" : "rnbo~ stereo-chorus",
					"varname" : "rnbo~[2]"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-21",
					"maxclass" : "slider",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 668.0, 31.0, 20.0, 140.0 ],
					"size" : 500.0
				}

			}
, 			{
				"box" : 				{
					"floatoutput" : 1,
					"id" : "obj-20",
					"maxclass" : "slider",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 903.0, 73.0, 20.0, 140.0 ],
					"size" : 1.0
				}

			}
, 			{
				"box" : 				{
					"attr" : "mix",
					"id" : "obj-19",
					"maxclass" : "attrui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 838.0, 269.0, 150.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-2",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "signal" ],
					"patching_rect" : [ 566.0, 203.0, 60.0, 22.0 ],
					"text" : "saw~ 500"
				}

			}
, 			{
				"box" : 				{
					"floatoutput" : 1,
					"id" : "obj-16",
					"maxclass" : "slider",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 88.0, 472.0, 20.0, 140.0 ],
					"size" : 1.0
				}

			}
, 			{
				"box" : 				{
					"floatoutput" : 1,
					"id" : "obj-15",
					"maxclass" : "slider",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 79.0, 282.0, 20.0, 140.0 ],
					"size" : 1.0
				}

			}
, 			{
				"box" : 				{
					"attr" : "q",
					"id" : "obj-14",
					"maxclass" : "attrui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 220.0, 434.0, 150.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"attr" : "cutoff",
					"id" : "obj-13",
					"maxclass" : "attrui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 220.0, 378.0, 150.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"floatoutput" : 1,
					"id" : "obj-11",
					"maxclass" : "slider",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 127.0, 137.0, 20.0, 140.0 ],
					"size" : 1.0
				}

			}
, 			{
				"box" : 				{
					"floatoutput" : 1,
					"id" : "obj-10",
					"maxclass" : "slider",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 354.0, 21.0, 20.0, 140.0 ],
					"size" : 1.0
				}

			}
, 			{
				"box" : 				{
					"attr" : "depth",
					"id" : "obj-9",
					"maxclass" : "attrui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 275.0, 320.0, 150.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"attr" : "rate",
					"id" : "obj-7",
					"maxclass" : "attrui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 275.0, 242.0, 150.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-8",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 0,
					"patching_rect" : [ 589.0, 776.0, 35.0, 22.0 ],
					"text" : "dac~"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-31", 0 ],
					"order" : 1,
					"source" : [ "obj-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-4", 0 ],
					"order" : 0,
					"source" : [ "obj-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-7", 0 ],
					"source" : [ "obj-10", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-9", 0 ],
					"source" : [ "obj-11", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-31", 0 ],
					"source" : [ "obj-13", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-31", 0 ],
					"source" : [ "obj-14", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-13", 0 ],
					"source" : [ "obj-15", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-14", 0 ],
					"source" : [ "obj-16", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-31", 0 ],
					"source" : [ "obj-19", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-31", 1 ],
					"order" : 0,
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-31", 0 ],
					"order" : 1,
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-19", 0 ],
					"source" : [ "obj-20", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 0 ],
					"source" : [ "obj-21", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-1", 0 ],
					"source" : [ "obj-3", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-32", 0 ],
					"order" : 1,
					"source" : [ "obj-31", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-5", 0 ],
					"order" : 0,
					"source" : [ "obj-31", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-8", 1 ],
					"order" : 1,
					"source" : [ "obj-31", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-8", 0 ],
					"order" : 0,
					"source" : [ "obj-31", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-31", 0 ],
					"source" : [ "obj-7", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-31", 0 ],
					"source" : [ "obj-9", 0 ]
				}

			}
 ],
		"originid" : "pat-84",
		"parameters" : 		{
			"obj-31" : [ "rnbo~[2]", "rnbo~[2]", 0 ],
			"obj-4" : [ "rnbo~", "rnbo~", 0 ],
			"parameterbanks" : 			{
				"0" : 				{
					"index" : 0,
					"name" : "",
					"parameters" : [ "-", "-", "-", "-", "-", "-", "-", "-" ]
				}

			}
,
			"inherited_shortname" : 1
		}
,
		"dependency_cache" : [ 			{
				"name" : "mono-chorus.maxsnap",
				"bootpath" : "~/Documents/Max 9/Snapshots",
				"patcherrelativepath" : "../../../../../Documents/Max 9/Snapshots",
				"type" : "mx@s",
				"implicit" : 1
			}
, 			{
				"name" : "mono-chorus.rnbopat",
				"bootpath" : "~/work/ol_dsp/modules/rnbo/patcher",
				"patcherrelativepath" : ".",
				"type" : "RBOP",
				"implicit" : 1
			}
, 			{
				"name" : "stereo-chorus.maxsnap",
				"bootpath" : "~/Documents/Max 9/Snapshots",
				"patcherrelativepath" : "../../../../../Documents/Max 9/Snapshots",
				"type" : "mx@s",
				"implicit" : 1
			}
, 			{
				"name" : "stereo-chorus.rnbopat",
				"bootpath" : "~/work/ol_dsp/modules/rnbo/patcher",
				"patcherrelativepath" : ".",
				"type" : "RBOP",
				"implicit" : 1
			}
 ],
		"autosave" : 0
	}

}
